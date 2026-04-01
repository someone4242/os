#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/tty.h>
#include "multiboot.h"

#define uint uint32_t

#define PAGE_SIZE 4096
#define TABLE_SIZE 1024
#define NB_PAGE (1 << 20)
#define PAGEID(ptr) ((int)((uintptr_t)(ptr) >> 12))

extern uint _kernel_start;
extern uint _kernel_end;
#define start_addr ((uint)&_kernel_start)
#define end_addr ((uint)&_kernel_end)

extern void loadPageDirectory(uint *);
extern void enablePaging();

static uint page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));

#define FREE 0
#define RESERVED 1

typedef struct physical_page_info {
    // 0 -> libre
    // 1 -> reservé
    uint8_t flags;
} physical_page_info;

static physical_page_info pageinfo[NB_PAGE] __attribute__((aligned(PAGE_SIZE)));

// we take the infos given by grub about memory map and fill in the page info table
static void init_physical_pageinfo(multiboot_info_t* mbd) {
    for(size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        for(uint addr = mmmt->addr_low; addr < mmmt->addr_low+mmmt->len_low; addr += PAGE_SIZE) {
            pageinfo[PAGEID(addr)].flags = mmmt->type - 1;
        }
    }
    for(uint addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
        pageinfo[PAGEID(addr)].flags = 1;
    }
}

bool is_free_physical_page(uint pageid) {
    return pageinfo[pageid].flags == FREE;
}

uintptr_t alloc_physical_page() {
    uintptr_t page_addr = 0;
    while(page_addr < NB_PAGE && !is_free_physical_page(PAGEID(page_addr)))
        page_addr += PAGE_SIZE;
    pageinfo[PAGEID(page_addr)].flags = RESERVED;
    return page_addr;
}

#define DEFAULT 0
static void virtual_memory_map(uint virtual_addr, uint physical_addr, uint8_t perm) {
    uint pagetable_id = virtual_addr >> 22;
    uint page_id = virtual_addr >> 12 & ((1 << 11) - 1);

    if((page_directory[pagetable_id] & 1) == 0) {
        // the page isn't present
        page_directory[pagetable_id] = alloc_physical_page() | perm | 3;
    }

    uint* pagetable_addr = (uint*)((page_directory[pagetable_id] >> 12) << 12);

    pagetable_addr[page_id] = physical_addr | perm | 3;
}

// we allocate the virtual addresses incrementally
uint brk = end_addr;
uint alloc_virtual_page(size_t memory_size) {
    uint nb_pages = ((uint)memory_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint virtual_addr = brk;
    brk += PAGE_SIZE * nb_pages;
    for(uint i_page = 0; i_page < nb_pages; i_page++) {
        uint physical_addr = (uint)alloc_physical_page();

        virtual_memory_map(virtual_addr + i_page * PAGE_SIZE, physical_addr, DEFAULT);
    }
    return virtual_addr;
}

void init_pagemap() {
    for (size_t i = 0; i < TABLE_SIZE; i++)
        page_directory[i] = 0x0002;
    
    for(uint addr = 0; addr < end_addr; addr += PAGE_SIZE)
        virtual_memory_map(addr, addr, DEFAULT);
}

void test_function(void);

void kernel_main(multiboot_info_t* mbd, uint magic) {
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
    printf("Le noyau occupe la RAM de %x à %x\n", start_addr, end_addr);
    for(size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        printf("Start Addr: %x %x | Length: %x %x | Size: %x | Type: %x\n",
            mmmt->addr_high, mmmt->addr_low, mmmt->len_high, mmmt->len_low, mmmt->size, mmmt->type);

        // if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
        //     /* 
        //      * Do something with this memory block!
        //      * BE WARNED that some of memory shown as availiable is actually 
        //      * actively being used by the kernel! You'll need to take that
        //      * into account before writing to memory!
        //      */
        // }
    }

    test_function();
}

int compare(const void* a, const void* b) {
    int* x = (int*) a;
    int* y = (int*) b;
    return *x - *y;
}

void test_function() {
    int n = 10000;
    int* a = (int*)malloc(n*sizeof(int));
    for(int i = 0; i < n; i++)
        a[i] = rand() % (2 * n);
    
    // for(int i = 0; i < n; i++)
    //     printf("%d ", a[i]);
    // printf("\n");
    qsort(a, n, sizeof(int), compare);
    // for(int i = 0; i < n; i++)
    //     printf("%d ", a[i]);
    // printf("\n");

    int nb_ok = 0;
    for(int i = 1; i < n; i++)
        if(a[i-1] <= a[i])
            nb_ok++;
    printf("%d\n", nb_ok);
}