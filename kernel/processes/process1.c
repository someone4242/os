#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/tty.h>

int main() {
    // terminal_set_vga();
    // terminal_initialize();
    // printf("test de processus\n");
    while(1) {
        asm volatile (" ");
        // asm volatile("int %0"
        //         : /* no result */
        //         : "i"(51)
        //         : "cc", "memory");
        void* a = malloc(sizeof(uint)*1000);
        printf("malloc\n");
        // asm volatile("int %0"
        //         : /* no result */
        //         : "i"(50)
        //         : "cc", "memory");
        // free(a);
    }
    return 0;
}
__attribute__((force_align_arg_pointer))
void _start(void) {
    // terminal_set_vga();
    main();
    while (1);
}
