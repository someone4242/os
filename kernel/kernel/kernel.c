#include <stdio.h>
#include "multiboot.h"
#include <stdint.h>
#include <kernel/tty.h>

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

void kernel_main(multiboot_info_t* mbd, uint32_t magic) {
	terminal_initialize();
	printf("Hello, world\n");
    /* Make sure the magic number matches for memory mapping*/
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        // panic("invalid magic number!");
		printf("invalid magic number!\n");
		return;
    }

    /* Check bit 6 to see if we have a valid memory map */
    if(!(mbd->flags >> 6 & 0x1)) {
        // panic("invalid memory map given by GRUB bootloader");
		printf("invalid memory map given by GRUB bootloader");
		return;
    }

    /* Loop through the memory map and display the values */
    int i;
    uint32_t start_addr = (uint32_t)&_kernel_start;
    uint32_t end_addr = (uint32_t)&_kernel_end;

    printf("Le noyau occupe la RAM de %d à %d\n", start_addr, end_addr);
    for(i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        printf("Start Addr: %u %u | Length: %u %u | Size: %u | Type: %u\n",
            mmmt->addr_high, mmmt->addr_low, mmmt->len_high, mmmt->len_low, mmmt->size, mmmt->type);

        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            /* 
             * Do something with this memory block!
             * BE WARNED that some of memory shown as availiable is actually 
             * actively being used by the kernel! You'll need to take that
             * into account before writing to memory!
             */
        }
    }
}