#ifndef __SANITIZATION_H
#define __SANITIZATION_H

#include <set>

// this is needed because dyninst as of now cannot resolve stuff like this
/*
0000000000003940 <main>:
    3940:       55                      push   %rbp
    3941:       48 89 e5                mov    %rsp,%rbp
    3944:       48 83 ec 20             sub    $0x20,%rsp
    3948:       64 48 8b 04 25 28 00    mov    %fs:0x28,%rax
    394f:       00 00
    3951:       48 89 45 f8             mov    %rax,-0x8(%rbp)
    3955:       c7 45 f4 00 00 00 00    movl   $0x0,-0xc(%rbp)
    395c:       89 7d f0                mov    %edi,-0x10(%rbp)
    395f:       48 89 75 e8             mov    %rsi,-0x18(%rbp)
    3963:       8b 7d f0                mov    -0x10(%rbp),%edi
    3966:       31 c9                   xor    %ecx,%ecx
    3968:       89 ca                   mov    %ecx,%edx
    396a:       e8 41 93 00 00          callq  ccb0 <pureftpd_start>
    396f:       64 48 8b 14 25 28 00    mov    %fs:0x28,%rdx
    3976:       00 00
    3978:       48 8b 75 f8             mov    -0x8(%rbp),%rsi
    397c:       48 39 f2                cmp    %rsi,%rdx
    397f:       89 45 e4                mov    %eax,-0x1c(%rbp)
    3982:       0f 85 09 00 00 00       jne    3991 <main+0x51>
    3988:       8b 45 e4                mov    -0x1c(%rbp),%eax
    398b:       48 83 c4 20             add    $0x20,%rsp
    398f:       5d                      pop    %rbp
    3990:       c3                      retq

    3991:       e8 da fa ff ff          callq  3470 <_init+0x160>
    3996:       66 2e 0f 1f 84 00 00    nopw   %cs:0x0(%rax,%rax,1)
    399d:       00 00 00

will continue to add blocks after this to <main> as it has no idea that
"callq  3470 <_init+0x160>" is a possibly exiting function
*/

// WARNING: will NOT update after patching!

namespace sanitization
{

void initialize(BPatch_image *);

bool check(BPatch_function *, BPatch_basicBlock *);

void apply(BPatch_function *, std::set<BPatch_basicBlock *> &);
};

#endif /* __SANITIZATION_H */
