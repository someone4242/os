#include <stdio.h>
#include "multiboot.h"
#include <stdint.h>
#include <kernel/tty.h>

#define PAGE_BYTE_SIZE 4096
#define PAGE_SIZE 1024
#define NB_PAGE (1 << 20)
#define PAGEID(ptr) ((int)((uintptr_t)(ptr) >> 12))

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;
uint32_t start_addr = (uint32_t)&_kernel_start;
uint32_t end_addr = (uint32_t)&_kernel_end;

extern void loadPageDirectory(uint32_t *);
extern void enablePaging();

static uint32_t page_directory[PAGE_SIZE] __attribute__((aligned(4096)));
static uint32_t first_page_table[PAGE_SIZE] __attribute__((aligned(4096)));
static uint32_t last_page_table[PAGE_SIZE] __attribute__((aligned(4096)));

#define FREE 0
#define RESERVED 1

typedef struct physical_page_info {
    // 0 -> libre
    // 1 -> reservé
    int8_t flags;
} physical_page_info;

static physical_page_info pageinfo[NB_PAGE];

// we take the infos given by grub about memory map and fill in the page info table
static void init_physical_pageinfo(multiboot_info_t* mbd) {
    uint32_t start_addr = (uint32_t)&_kernel_start;
    uint32_t end_addr = (uint32_t)&_kernel_end;
    for(size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        for(uint32_t addr = mmmt->addr_low; addr < mmmt->addr_low+mmmt->len_low; addr += PAGE_BYTE_SIZE) {
            pageinfo[PAGEID(addr)].flags = mmmt->type - 1;
        }
    }
    for(uint32_t addr = start_addr; addr < end_addr; addr += PAGE_BYTE_SIZE) {
        pageinfo[PAGEID(addr)].flags = 1;
    }
}

int is_free_physical_page(uint32_t pageid) {
    return pageinfo[pageid].flags == FREE;
}

static void reserve_physical_page(int pageid) {
    pageinfo[pageid].flags = RESERVED;
}

uintptr_t get_free_physical_page() {
    uintptr_t page_addr = 0;
    while(page_addr < NB_PAGE && !is_free_physical_page(PAGEID(page_addr)))
        page_addr += PAGE_BYTE_SIZE;
    reserve_physical_page(PAGEID(page_addr));
    return page_addr;
}

static void identity_mapping(uint32_t page_addr) {
    uint32_t pagetable_id = page_addr >> 22;
    uint32_t page_id = page_addr >> 12 & ((1 << 11) - 1);

    if((page_directory[pagetable_id] & 1) == 0) {
        // the page isn't present
        page_directory[pagetable_id] = get_free_physical_page() | 3;
    }

    uint32_t* pagetable_addr = (uint32_t*)((page_directory[pagetable_id] >> 12) << 12);

    pagetable_addr[page_id] = page_addr | 3;
}

void init_pagemap() {
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        *(page_directory + i) = 0x0002;
    }
    
    for(uint32_t addr = 0; addr < end_addr; addr += PAGE_BYTE_SIZE)
        identity_mapping(addr);
}

void kernel_main(multiboot_info_t* mbd, uint32_t magic) {
	terminal_initialize();
	printf("Hello, world\n");
    // make sure the magic number matches for memory mapping
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        // panic("invalid magic number!");
		printf("invalid magic number!\n");
		return;
    }

    // check bit 6 to see if we have a valid memory map 
    if(!(mbd->flags >> 6 & 0x1)) {
        // panic("invalid memory map given by GRUB bootloader");
		printf("invalid memory map given by GRUB bootloader");
		return;
    }

    // init physical allocator
    init_physical_pageinfo(mbd);

    // init and enable paging
    init_pagemap();
    loadPageDirectory(page_directory);
    enablePaging();


    // loop through the memory map and display the values 
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
