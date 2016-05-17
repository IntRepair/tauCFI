#include <stdio.h>
#include "types.h"
#include "syscall.h"

SYSCALL_ENTRY syscalls[] = {
#include "syscall.def"
};


const SYSCALL_ENTRY* FindSyscallByNum(unsigned int syscall_num)
{
#if 0
    unsigned int s = 0;
    unsigned int e = sizeof(syscalls) / sizeof(syscalls[0]);

    while (s < e) {
        unsigned int i = (s + e) >> 1;
        int num = syscalls[i].syscall_num;

        if (num == syscall_num)
            return &syscalls[i];
        else if (num < syscall_num)
            s = i + 1;
        else
            e = i;
    }
    return NULL;
#else
    for (int i = 0; i < sizeof(syscalls) / sizeof(SYSCALL_ENTRY); i++) {
        if (syscall_num == syscalls[i].syscall_num)
            return &syscalls[i];
    }
    return NULL;
#endif
}


