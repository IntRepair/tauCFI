/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2012 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/// @file xed-ex6.c
/// @author Mark Charney   <mark.charney@intel.com>

// decoder->encoder example. 

#include "xed-interface.h"
#include "xed-examples-util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strcmp
#include <assert.h> 

int main(int argc, char** argv);

int 
main(int argc, char** argv) {
    xed_error_enum_t xed_error;
    xed_bool_t long_mode = 0;
    xed_bool_t prot16 = 0;
    unsigned int first_argv;
    unsigned int bytes = 0;
    unsigned char itext[XED_MAX_INSTRUCTION_BYTES];
    int i;
    unsigned int u;
    xed_decoded_inst_t xedd;
#define BUFLEN  1000
    char buffer[BUFLEN];
    xed_bool_t ok;
    unsigned int isyntax;
    xed_syntax_enum_t syntax;
    xed_machine_mode_enum_t mmode;
    xed_address_width_enum_t stack_addr_width;
    xed_encoder_request_t* enc_req;
    xed_uint8_t array[XED_MAX_INSTRUCTION_BYTES];
    unsigned int enc_olen, ilen = XED_MAX_INSTRUCTION_BYTES;
    xed_error_enum_t encode_okay;
    xed_bool_t change_to_long_mode = 0;

    xed_tables_init();
    xed_set_verbosity( 99 );

    if (argc > 2 && strcmp(argv[1], "-64") == 0) 
        long_mode = 1;
    else if (argc > 2 && strcmp(argv[1], "-16") == 0) 
        prot16 = 1;

    if (long_mode) {
        first_argv = 2;
        mmode=XED_MACHINE_MODE_LONG_64;
        stack_addr_width =XED_ADDRESS_WIDTH_64b;
    }
    else if (prot16) {
        first_argv = 2;
        mmode=XED_MACHINE_MODE_LEGACY_16;
        stack_addr_width =XED_ADDRESS_WIDTH_16b;
    }
    else {
        first_argv=1;
        mmode=XED_MACHINE_MODE_LEGACY_32;
        stack_addr_width =XED_ADDRESS_WIDTH_32b;
    }

    xed_decoded_inst_zero(&xedd);
    xed_decoded_inst_set_mode(&xedd, mmode, stack_addr_width);

    for( i=first_argv ;i < argc; i++)    {
        xed_uint8_t x = (xed_uint8_t)(xed_atoi_hex(argv[i]));
        assert(bytes < XED_MAX_INSTRUCTION_BYTES);
        itext[bytes++] = x;
    }
    if (bytes == 0)    {
        fprintf(stderr, "Must supply some hex bytes\n");
        exit(1);
    }

    printf("PARSING BYTES: ");
    for( u=0;u<bytes; u++) 
        printf("%02x ", XED_STATIC_CAST(unsigned int,itext[u]));
    printf("\n");

    xed_error = xed_decode(&xedd, 
                           XED_REINTERPRET_CAST(const xed_uint8_t*,itext),
                           bytes);
    switch(xed_error)
    {
      case XED_ERROR_NONE:
        break;
      case XED_ERROR_BUFFER_TOO_SHORT:
        fprintf(stderr,"Not enough bytes provided\n");
        exit(1);
      case XED_ERROR_GENERAL_ERROR:
        fprintf(stderr,"Could not decode given input.\n");
        exit(1);
      default:
        fprintf(stderr,"Unhandled error code %s\n", xed_error_enum_t2str(xed_error));
        exit(1);
    }
        
    //memset(buffer,0,BUFLEN);
    xed_decoded_inst_dump(&xedd,buffer, BUFLEN);
    printf("%s\n",buffer);

    for(isyntax=  XED_SYNTAX_XED; isyntax < XED_SYNTAX_LAST; isyntax++)    {
        syntax = XED_STATIC_CAST(xed_syntax_enum_t, isyntax);
        ok = xed_format(syntax, &xedd, buffer, BUFLEN, 0);
        if (ok)
            printf("%s syntax: %s\n", xed_syntax_enum_t2str(syntax), buffer);
        else
            printf("Error disassembling %s syntax\n", xed_syntax_enum_t2str(syntax));
    }

    printf("\nPreparing to encode ...\n");
    enc_req = &xedd;  // they are basically the same now
    // convert decode structure to proper encode structure
    xed_encoder_request_init_from_decode(&xedd);

    change_to_long_mode = 0;
    if (change_to_long_mode) {
        // change to 64b mode
        xed_state_t state;
        xed_operand_values_t* ov;
        xed_state_init2(&state,XED_MACHINE_MODE_LONG_64,XED_ADDRESS_WIDTH_64b);
        ov = xed_decoded_inst_operands(&xedd);
        xed_operand_values_set_mode(ov, &state);
        xed_encoder_request_set_effective_address_size(enc_req, 64);
        // need to fix base regs...
        //xed_operand_values_set_operand_reg(ov, XED_OPERAND_BASE0, XED_REG_RSI);
        xed_encoder_request_set_effective_operand_width(enc_req, 32);
    }


    printf("Encoding...\n");
    encode_okay =  xed_encode(enc_req, array, ilen, &enc_olen);
    if (encode_okay != XED_ERROR_NONE) {
        printf("Error code = %s\n", xed_error_enum_t2str(encode_okay));
    }
    else {
        unsigned int j;
        printf("Encodable: ");
        for(j=0;j<enc_olen;j++) {
            printf("%02x", array[j]);
        }
        printf("\n");
    }

    return 0;
}
