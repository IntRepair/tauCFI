/*
 * See the dyninst/COPYRIGHT file for copyright information.
 *
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 *
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "InstructionDecoderImpl.h"
#include <iostream>
#include "Immediate.h"
#include "dyn_regs.h"

namespace Dyninst {
    namespace InstructionAPI
    {

#if defined(__GNUC__)
#define insn_printf(format, ...) \
        do{ \
            printf("[%s:%u]insn_debug " format, FILE__, __LINE__, ## __VA_ARGS__); \
        }while(0)
#endif

#define AARCH64_INSN_LENGTH	32

        struct aarch64_insn_entry;
		struct aarch64_mask_entry;

        class InstructionDecoder_aarch64 : public InstructionDecoderImpl
        {
            friend struct aarch64_insn_entry;
			friend struct aarch64_mask_entry;

            public:
                InstructionDecoder_aarch64(Architecture a);
                virtual ~InstructionDecoder_aarch64();
                virtual void decodeOpcode(InstructionDecoder::buffer& b);
                virtual Instruction::Ptr decode(InstructionDecoder::buffer& b);
				virtual void setMode(bool) {}
                virtual bool decodeOperands(const Instruction* insn_to_complete);
                virtual void doDelayedDecode(const Instruction* insn_to_complete);

                static std::vector<std::string> condStringMap;
                static std::map<unsigned int, MachRegister> sysRegMap;

                #define	IS_INSN_LOGICAL_SHIFT(I)		(field<24, 28>(I) == 0x0A)
                #define	IS_INSN_ADDSUB_EXT(I)			(field<24, 28>(I) == 0x0B && field<21, 21>(I) == 1)
                #define	IS_INSN_ADDSUB_SHIFT(I)			(field<24, 28>(I) == 0x0B && field<21, 21>(I) == 0)
                #define	IS_INSN_ADDSUB_IMM(I)			(field<24, 28>(I) == 0x11)

                //----ld/st-----
                #define IS_INSN_LDST(I)                 (field<25, 25>(I) == 0x00 && field<27, 27>(I) == 1)

                #define IS_INSN_LDST_EX(I)              (field<24, 29>(I) == 0x08)
                #define IS_INSN_ST_EX(I)                (field<24, 29>(I) == 0x08 && field<22, 22>(I) == 0)
                #define IS_INSN_LDST_EX_PAIR(I)         (field<24, 29>(I) == 0x08 && field<21, 21>(I) ==0x01)

                #define IS_INSN_LD_LITERAL(I)           (field<27, 29>(I) == 0x03 && field<24, 25>(I) == 0)

                #define IS_INSN_LDST_PAIR(I)            (field<27, 29>(I) == 0x05)
                #define IS_INSN_LDST_PAIR_PRE(I)        (field<27, 29>(I) == 0x05 && field<23, 25>(I) == 0x03)
                #define IS_INSN_LDST_PAIR_OFFSET(I)     (field<27, 29>(I) == 0x05 && field<23, 25>(I) == 0x02)
                #define IS_INSN_LDST_PAIR_POST(I)       (field<27, 29>(I) == 0x05 && field<23, 25>(I) == 0x01)
                #define IS_INSN_LDST_PAIR_NOALLOC(I)    (field<27, 29>(I) == 0x05 && field<23, 25>(I) == 0x00)

                #define IS_INSN_LDST_UNSCALED(I)        (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 0 && field<21, 21>(I) == 0 && field<10, 11>(I) == 0x00)
                #define IS_INSN_LDST_POST(I)            (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 0 && field<21, 21>(I) == 0 && field<10, 11>(I) == 0x01)
                #define IS_INSN_LDST_UNPRIV(I)          (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 0 && field<21, 21>(I) == 0 && field<10, 11>(I) == 0x02)
                #define IS_INSN_LDST_PRE(I)             (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 0 && field<21, 21>(I) == 0 && field<10, 11>(I) == 0x03)
                #define	IS_INSN_LDST_REG(I)	            (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 0 && field<21, 21>(I) == 1 && field<10, 11>(I) == 0x02)
                #define IS_INSN_LDST_UIMM(I)            (field<27, 29>(I) == 0x07 && field<24, 25>(I) == 1)


                #define	IS_INSN_LOGICAL_IMM(I)			(field<23, 28>(I) == 0x24)
                #define	IS_INSN_BITFIELD(I)				(field<23, 28>(I) == 0x26)
                #define	IS_INSN_EXCEPTION(I)			(field<24, 31>(I) == 0xD4)
                #define	IS_INSN_FP_COMPARE(I)			(field<24, 28>(I) == 0x1E && field<21, 21>(I) == 0x1 && field<10, 13>(I) == 0x8)
                #define	IS_INSN_FP_CONV_FIX(I)			(field<24, 28>(I) == 0x1E && field<21, 21>(I) == 0x0)
                #define	IS_INSN_FP_CONV_INT(I)			(field<24, 28>(I) == 0x1E && field<21, 21>(I) == 0x1 && field<10, 15>(I) == 0x0)
                #define	IS_SOURCE_GP(I)					(field<16, 18>(I) == 0x2  || field<16, 18>(I) == 0x3 || field<16, 18>(I) == 0x7)
				#define	IS_INSN_FP_DATAPROC_ONESRC(I)	(field<24, 28>(I) == 0x1E && field<21, 21>(I) == 0x1 && field<10, 14>(I) == 0x10)
				#define	IS_INSN_B_UNCOND(I)				(field<26, 30>(I) == 0x05)
                #define	IS_INSN_B_UNCOND_REG(I)			(field<25, 31>(I) == 0x6B)
                #define	IS_INSN_B_COMPARE(I)			(field<25, 30>(I) == 0x1A)
                #define	IS_INSN_B_TEST(I)				(field<25, 30>(I) == 0x1B)
                #define	IS_INSN_B_COND(I)				(field<25, 31>(I) == 0x2A)
                #define	IS_INSN_PCREL_ADDR(I)			(field<24, 28>(I) == 0x10)
                #define	IS_INSN_SYSTEM(I)				(field<22, 31>(I) == 0x354)
                #define	IS_INSN_BRANCHING(I)			(IS_INSN_B_COND(I) || IS_INSN_B_UNCOND(I) || IS_INSN_B_UNCOND_REG(I) || IS_INSN_B_TEST(I) 										 || IS_INSN_B_COMPARE(I))

                #define	IS_FIELD_IMMR(S, E)				(S == 16 && E == 21)
                #define	IS_FIELD_IMMS(S, E)				(S == 10 && E == 15)
                #define	IS_FIELD_IMMLO(S, E)			(S == 29 && E == 30)
                #define	IS_FIELD_IMMHI(S, E)			(S == 5 && E == 23)

            private:
                virtual Result_Type makeSizeType(unsigned int opType);

				bool isPstateRead, isPstateWritten;
                bool isFPInsn, isSIMDInsn;
                bool is64Bit;
                bool isValid;

                void mainDecode();
                int findInsnTableIndex(unsigned int);

				/*members for handling operand re-ordering, will be removed later once a generic operand ordering method is incorporated*/
				int oprRotateAmt;
				bool hasb5;
				void reorderOperands();

                static void buildSysRegMap();
                unsigned int insn;
                Instruction* insn_in_progress;

                template <int start, int end>
                int field(unsigned int raw)
                {
#if defined DEBUG_FIELD
                    std::cerr << start << "-" << end << ":" << std::dec << (raw >> (start) &
                            (0xFFFFFFFF >> (31 - (end - start)))) << " ";
#endif
                    return (raw >> (start) & (0xFFFFFFFF >> (31 - (end - start))));
                }

                int32_t sign_extend32(int size, int in)
                {
					int32_t val = 0|in;

                    return (val << (32 - size)) >> (32 - size);
                }

                int64_t sign_extend64(int size, int in)
                {
					int64_t val = 0|in;

                    return (val << (64 - size)) >> (64 - size);
                }

                uint32_t unsign_extend32(int size, int in)
                {
					uint32_t mask = ~0;

                    return (mask>>(32-size)) & in;
				}

                uint64_t unsign_extend64(int size, int in)
                {
					uint64_t mask = ~0;

                    return (mask>>(64-size)) & in;
				}

				int op1Field, op2Field, crmField;
				void processSystemInsn();

				bool hasHw;
				int hwField;
				void processHwFieldInsn(int, int);

				bool hasShift;
				int shiftField;
				void processShiftFieldShiftedInsn(int, int);
				void processShiftFieldImmInsn(int, int);

				bool hasOption;
				int optionField;
				void processOptionFieldLSRegOffsetInsn();

				bool hasN;
				int immr, immrLen;
				int sField, nField, nLen;

        		int immlo, immloLen;
				void makeBranchTarget(bool, bool, int, int);
				Expression::Ptr makeFallThroughExpr();

                int _szField;
                int _typeField;

                void NOTHING();

                void set32Mode();
				void setRegWidth();
				void setFPMode();
				void setSIMDMode();

                bool isSinglePrec();

				MachRegister makeAarch64RegID(MachRegister, unsigned int);
				Expression::Ptr makeRdExpr();
				Expression::Ptr makeRnExpr();
				Expression::Ptr makeRmExpr();
				Expression::Ptr makeRaExpr();
				Expression::Ptr makeRsExpr();
				Expression::Ptr makePstateExpr();
                Expression::Ptr makePCExpr();
                Expression::Ptr makeb40Expr();
				Expression::Ptr makeOptionExpression(int, int);
				template<typename T, Result_Type rT>
				Expression::Ptr fpExpand(int);
                Expression::Ptr makeRtExpr();
                Expression::Ptr makeRt2Expr();

                Expression::Ptr makeMemRefReg();
                Expression::Ptr makeMemRefReg_Rm();
                Expression::Ptr makeMemRefReg_ext();
                Expression::Ptr makeMemRefReg_amount();

                Expression::Ptr makeMemRefIndexLiteral();
                Expression::Ptr makeMemRefIndexUImm();
                Expression::Ptr makeMemRefIndexPre();
                Expression::Ptr makeMemRefIndexPost();
                Expression::Ptr makeMemRefIndex_addOffset9();
                Expression::Ptr makeMemRefIndex_offset9();

                Expression::Ptr makeMemRefPairPre();
                Expression::Ptr makeMemRefPairPost();
                Expression::Ptr makeMemRefPair_offset7();
                Expression::Ptr makeMemRefPair_addOffset7();
                Expression::Ptr makeMemRefEx();
                Expression::Ptr makeMemRefExPair();
                Expression::Ptr makeMemRefExPair2();
                //Expression::Ptr makeMemRefPairPre2();
                //Expression::Ptr makeMemRefPairPost2();
                //Expression::Ptr makeMemRefExPair16B();


                void getMemRefIndexLiteral_OffsetLen(int &, int &);
                void getMemRefIndex_SizeSizelen(unsigned int &, unsigned int &);
                void getMemRefIndexPrePost_ImmImmlen(unsigned int& , unsigned int& );
                void getMemRefPair_ImmImmlen(unsigned int &immVal, unsigned int &immLen);

                void getMemRefEx_RT(Result_Type &rt);
                void getMemRefIndexLiteral_RT(Result_Type &);
                void getMemRefExPair_RT(Result_Type &rt);
                void getMemRefPair_RT(Result_Type &rt);
                void getMemRefIndex_RT(Result_Type &);
                void getMemRefIndexUImm_RT(Result_Type &);

				void OPRRd();
				void OPRsf();
				template<unsigned int endBit, unsigned int startBit>
                void OPRoption();
				void OPRshift();
				void OPRhw();
				template<unsigned int endBit, unsigned int startBit>
                void OPRN();

                //for load store
                void LIndex();
                void STIndex();
				void OPRRn();
                void OPRRnL();
                void OPRRnLU();
                void OPRRnSU();
                void OPRRnS();
				void OPRRnU();
				void OPRRm();
				void OPRRt();
				void OPRRtL();
				void OPRRtS();
				void OPRRt2();
				void OPRRt2L();
				void OPRRt2S();

				void OPRop1();
				void OPRop2();
				template<unsigned int endBit, unsigned int startBit>
                void OPRcond();
				void OPRnzcv();
				void OPRCRm();
				void OPRCRn();
				template<unsigned int endBit, unsigned int startBit>
                void OPRS();
				void OPRRa();
				void OPRo0();
				void OPRb5();
				void OPRb40();
				template<unsigned int endBit, unsigned int startBit>
				void OPRsz();
				void OPRRs();
				template<unsigned int endBit, unsigned int startBit>
                void OPRimm();
				void OPRscale();
				template<unsigned int endBit, unsigned int startBit>
                void OPRtype();

                void OPRH() {}
                void OPRL() {}
                void OPRM() {}
                void OPRQ() {}
                void OPRa() {}
                void OPRb() {}
                void OPRc() {}
                void OPRd() {}
                void OPRe() {}
                void OPRf() {}
                void OPRg() {}
                void OPRh() {}
                void OPRopc() {}
                void OPRopcode() {}
                void OPRlen() {}
                template<unsigned int endBit, unsigned int startBit>
                void OPRsize() {}
                void OPRcmode() {}
                void OPRrmode() {}
                void OPRop() {}
                void setFlags();
        };
    }
}
