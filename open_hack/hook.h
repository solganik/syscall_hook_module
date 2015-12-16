#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <linux/kernel.h>
#include <linux/module.h>

typedef asmlinkage int (*open_syscall_type)(const char *, int, int);

open_syscall_type hook_open(open_syscall_type cb);
void unhook_open(open_syscall_type original);
#endif
