#include <stdio.h>
#include "multiboot.h"
#include <stdint.h>
#include <kernel/tty.h>

#define PAGE_BYTE_SIZE 4096
#define PAGE_SIZE 1024

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

extern void loadPageDirectory(uint32_t *);
extern void enablePaging();

void init_identity_mapping(uint32_t page_directory[], size_t identity_pages) {
    uint32_t first_page_table[PAGE_SIZE] __attribute__((aligned(4096)));

    for (size_t i = 0; i < PAGE_SIZE; i++) {
        *(page_directory + i) = 0x0002;
    }
    for (size_t i = 0; i < identity_pages; i++) {
        *(first_page_table + i) = (i * 0x1000) | 3;
    }

    *(page_directory + 0) = ((uint32_t)first_page_table) | 3;
}

void kernel_main(multiboot_info_t* mbd, uint32_t magic) {
    uint32_t start_addr = (uint32_t)&_kernel_start;
    uint32_t end_addr = (uint32_t)&_kernel_end;

    uint32_t page_directory[PAGE_SIZE] __attribute__((aligned(4096)));
    size_t identity_pages = end_addr >> 12;

    init_identity_mapping(page_directory, identity_pages);
    loadPageDirectory(page_directory);
    enablePaging();


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

    printf("Le noyau occupe la RAM de %d à %d\n", start_addr, end_addr);
    for(size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
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
