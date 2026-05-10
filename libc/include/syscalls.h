#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stdint.h>

#define SYSCALL_TEST 48

#define SYSCALL_TERMLOAD 100

void sys_test(void);

void sys_termload(const void *data);


#endif
