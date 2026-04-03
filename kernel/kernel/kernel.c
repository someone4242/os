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
    uintptr_t page_addr = end_addr;
    while(page_addr < NB_PAGE && !is_free_physical_page(PAGEID(page_addr)))
        page_addr += PAGE_SIZE;

    uint* page = (uint*)page_addr;
    for(int i = 0; i < TABLE_SIZE; i++)
        page[i] = 0;

    pageinfo[PAGEID(page_addr)].flags = RESERVED;
    return page_addr;
}

#define DEFAULT 0

static uint page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
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

static uint first_pagetable[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
void init_pagemap() {
    for (size_t i = 0; i < TABLE_SIZE; i++)
        page_directory[i] = 0x0002;
    
    page_directory[0] = (uint)&first_pagetable | 3;
    for(uint addr = 0; addr < end_addr + PAGE_SIZE * 50; addr += PAGE_SIZE)
        virtual_memory_map(addr, addr, DEFAULT);
}

void test_function(void);




/*
 * Setting up the Global Descriptor Table (GDT)
 */
#define GDT_SIZE 5
typedef struct {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} __attribute((packed)) gdt_desc_t;
typedef struct {
    uint16_t    size;
    uint32_t    offset;
} __attribute__((packed)) gdtr_t;

static gdt_desc_t gdt[GDT_SIZE];
static gdtr_t gdtr;

extern void gdt_flush();

static void setGdtEntry(int num, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran) {
    // limit should be <= 0xFFFFF

    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit = (limit & 0xFFFF);
    gdt[num].flags = (limit >> 16) & 0x0F;
    gdt[num].flags |= (gran & 0xF0);

    gdt[num].access = access;
}

void init_gdt() {
    gdtr.size = GDT_SIZE * sizeof(gdt_desc_t) - 1;
    gdtr.offset = (uint32_t)&gdt;

    setGdtEntry(0, 0, 0, 0, 0); // Null
    setGdtEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code
    setGdtEntry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data
    setGdtEntry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code
    setGdtEntry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data

    asm volatile ("lgdt %0" : : "m"(gdtr));
    gdt_flush();
}


/*
 * Setting up the Interrupt Descriptor Table (IDT)
 */

#define IDT_SIZE 256
typedef struct {
    uint16_t offset_low;
    uint16_t seg;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_desc_t;
typedef struct {
    uint16_t    size;
    uint32_t    offset;
} __attribute__((packed)) idtr_t;

static idt_desc_t idt[IDT_SIZE];
static idtr_t idtr;
extern void isr_stub_0();

void setIdtEntry(uint8_t vector, void *isr, uint8_t flags) {
    idt_desc_t *desc =(idt_desc_t *)&idt[vector];
    desc->offset_low = (uint32_t)isr & 0xFFFF;
    desc->offset_high = ((uint32_t)isr >> 16) & 0xFFFF;
    desc->seg = 0x08;
    desc->flags = flags;
    desc->reserved = 0;
}

void init_idt() {
    idtr.size = IDT_SIZE * sizeof(idt_desc_t) - 1;
    idtr.offset = (uint32_t)&idt;

    for (size_t i =0; i < IDT_SIZE; i++)
        setIdtEntry(i, isr_stub_0 + (i*16), 0b10001110);

    asm volatile ("lidt %0" : : "m"(idtr));
}


void interrupt_dispatch(uint32_t int_num, uint32_t err_code) {
    printf("\nINT %d; err_code %x\n", int_num, err_code);
    switch (int_num)
    {
        case 0:
            printf("Division by 0\n");
            break;
        default:
            printf("Unhandled interrupt\n");
            break;
    }
    printf("Interrupt handled, halting...\n");
    while (1) {};
}



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
    asm volatile("sti");

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
    printf("\n");
    int n = 10;
    int* a = (int*)malloc(n*sizeof(int));
    for(int i = 0; i < n; i++)
        a[i] = rand() % (2 * n);
    
    qsort(a, n, sizeof(int), compare);

    bool ok = true;
    for(int i = 1; i < n; i++)
        ok &= (a[i-1] <= a[i]);
    printf("quicksort is ok ? %s\n", (ok ? "ok" : "not ok"));
}
