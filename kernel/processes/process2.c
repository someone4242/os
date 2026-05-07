#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/tty.h>

int main() {
    // terminal_set_vga();
    // terminal_initialize();
    // printf("test de processus\n");
    // test_function();
    while(1) {
        asm volatile (" ");
        // asm volatile("int %0"
        //         : /* no result */
        //         : "i"(49)
        //         : "cc", "memory");
    }
    return 0;
}