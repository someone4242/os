#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/allocator.h>
#include <kernel/interrupt.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>
#include <kernel/kernel.h>
#include <kbdriver.h>

void init_pagemap() {
    for (size_t i = 0; i < TABLE_SIZE; i++)
        page_directory[i] = 0x0002;
    
    page_directory[0] = (uint)&first_pagetable | 3;
    page_directory[TABLE_SIZE-1] = (uint)page_directory | 3;
    for(uint ptx = 0; ptx < TABLE_SIZE; ptx++)
        first_pagetable[ptx] = (ptx << 12) | 3;
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

    // init GDT, IDT and enable interrupts
    init_gdt();
    init_idt();
    asm volatile ("sti");

    init_kbdriver();

    // init physical allocator
    init_physical_pageinfo(mbd);

    // init and enable paging
    init_pagemap();
    loadPageDirectory(page_directory);
    enablePaging();

    SAFE_BLOCK_NUMBER_public_edit(rand());

    printf("kernel: %x - %x\n", start_addr, end_addr);
    printf("pageinfo: %x - %x\n", (uint)pageinfo, (uint)pageinfo + sizeof(pageinfo));
    printf("page_directory: %x\n", (uint)page_directory);
    printf("first_pagetable: %x\n", (uint)first_pagetable);
    printf("brk: %x\n", brk_public_get());

    printf("keyboard set : %d\n", get_keyboard_set());


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
    /*
    test_function();

    int n = 10;
    uint** ptr = (uint**)malloc(sizeof(uint*) * n);
    for(int i = 0; i < n; i++) {
        uint sz = (rand() % 500000);
        printf("asked for %d bytes\n", sizeof(uint)*sz);
        ptr[i] = (uint*)malloc(sizeof(uint)*sz);
        test_function();
    }

    int p[] = {1, 7, 9, 0, 8, 4, 2, 5, 6, 3};
    for(int i = 0; i < n; i++) {
        free(ptr[p[i]]);
        printf("current free list state :\n");
        print_memory(first_block);
        printf("\n");
    }

    */
    while (1); // à garder, si aucun processus implémenté
}

int compare(const void* a, const void* b) {
    int* x = (int*) a;
    int* y = (int*) b;
    return *x - *y;
}

void test_function() {
    // printf("\n");
    int n = 1000000;
    int* a = (int*)malloc(n*sizeof(int));

    // printf("adresse du tableau : %d\n", (uint)a);
    for(int i = 0; i < n; i++)
        a[i] = rand() % (2 * n);
    
    qsort(a, n, sizeof(int), compare);

    bool ok = true;
    for(int i = 1; i < n; i++)
        ok &= (a[i-1] <= a[i]);
    //feur();
    printf("quicksort is ok ? %s\n", (ok ? "ok" : "not ok"));
    

    // printf("pointeur du tableau : %d\n", (uint)a);
    free(a);
}
