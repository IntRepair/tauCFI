#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "types.h"

struct SYSCALL_ENTRY {
  int syscall_num;
  const char* syscall_name;
  int syscall_ret;
  int syscall_arg;
  int syscall_args[20];
};

const SYSCALL_ENTRY* FindSyscallByNum(unsigned int syscall_num);

#endif
