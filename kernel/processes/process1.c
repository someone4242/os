#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/tty.h>

int main() {
    terminal_initialize();
    // printf("test de processus\n");
    asm volatile("int %0"
            : /* no result */
            : "i"(48)
            : "cc", "memory");
    while(1) {
        asm volatile("");
    }
    return 0;
}