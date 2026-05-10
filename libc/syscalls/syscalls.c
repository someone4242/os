/* SYSCALLS */
#include <syscalls.h>


void sys_test(void) {
    asm volatile("int %0"
            : /* no result */
            : "i"(SYSCALL_TEST)
            : "cc", "memory");
}



void sys_termload(const void *data) {
    asm volatile("int %0"
            : /* no result */
            : "i"(SYSCALL_TERMLOAD), "D" (data)
            : "cc", "memory");
}

