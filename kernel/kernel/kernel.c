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
#define PAGEID(ptr) ((uint)((uintptr_t)(ptr) >> 12))

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
            pageinfo[PAGEID(addr)].flags = (mmmt->type == 1 ? FREE : RESERVED);
        }
    }
    for(uint addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
        pageinfo[PAGEID(addr)].flags = 1;
    }
}

bool is_free_physical_page(uint pageid) {
    return pageinfo[pageid].flags == FREE;
}

uint pageid = NB_PAGE;
static uint* const recursive_pd = (uint*)0xFFFFF000;
static uint* const recursive_pt = (uint*)0xFFC00000;

uintptr_t alloc_physical_page() {
    if(pageid == NB_PAGE) pageid = PAGEID(end_addr);
    uint cnt = 0;
    while(cnt < NB_PAGE && !is_free_physical_page(pageid)) {
        pageid++, cnt++;
        if(pageid == PAGEID((uint)recursive_pt))
            pageid = PAGEID(end_addr);
    }
    if(cnt == NB_PAGE) {
        // panic : memory limit exceeded
        printf("memory limit exceeded\n");
        return 0;
    }

    pageinfo[pageid].flags = RESERVED;
    return (uintptr_t)(pageid * PAGE_SIZE);
}

#define DEFAULT 0

static uint page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static void virtual_memory_map(uint virtual_addr, uint physical_addr, uint8_t perm) {
    uint pdx = virtual_addr >> 22;

    if((recursive_pd[pdx] & 1) == 0) {
        // the page isn't present
        recursive_pd[pdx] = alloc_physical_page() | perm | 3;

        // we clean up the table
        uint* table_virt = (uint*)(0xFFC00000 + (pdx << 12));
        for(int i = 0; i < 1024; i++) table_virt[i] = 0;
    }

    recursive_pt[virtual_addr >> 12] = physical_addr | perm | 3;
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

// we allocate the virtual addresses incrementally
uint brk = TABLE_SIZE * PAGE_SIZE;
uint alloc_virtual_page(size_t memory_size) {
    uint nb_pages = ((uint)memory_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint virtual_addr = brk;
    printf("virtual allocation : from %d", brk);
    brk += PAGE_SIZE * nb_pages;
    printf(" to %d\n", brk);
    for(uint i_page = 0; i_page < nb_pages; i_page++) {
        uint physical_addr = (uint)alloc_physical_page();

        virtual_memory_map(virtual_addr + i_page * PAGE_SIZE, physical_addr, DEFAULT);
    }
    return virtual_addr;
}

static uint SAFE_BLOCK_NUMBER = 0x15366593;

typedef struct block {
    size_t size;
    bool free;
    uint magic_number;
    struct block* next;
    struct block* prev;
} block;

static block* first_block = NULL;
static block* last_block = NULL;

void print_memory(block* bloc) {
    if(bloc == NULL) return;
    printf("ptr : %d, size : %d, free : %s, next : %d, prev : %d\n", (uint)bloc, bloc->size, (bloc->free ? "yes" : "no"), (uint)bloc->next, (uint)bloc->prev);
    print_memory(bloc->next);
}

block* get_new_block(size_t memory_size) {
    block* ptr = (block*)alloc_virtual_page(memory_size + sizeof(block));
    ptr->size = memory_size;
    ptr->free = true;
    ptr->next = NULL;
    ptr->prev = NULL;
    ptr->magic_number = SAFE_BLOCK_NUMBER;
    return ptr;
}

void push_back(block* cur_block) {
    if(first_block == NULL) {
        last_block = first_block = cur_block;
    } else {
        last_block->next = cur_block;
        cur_block->prev = last_block;
        last_block = cur_block;
    }
}

void add_block(size_t memory_size) {
    size_t mem_alloc = memory_size + (sizeof(int) * (1 << 20));
    block* ptr = get_new_block(mem_alloc);

    // printf("first block %d, last block %d\n", (uint)first_block, (uint)last_block);
    push_back(ptr);
}

block* split_block(block* cur_block, size_t memory_size) {
    // printf("trying to cut in half\n");
    block* new_block = (block*)((uint)cur_block + cur_block->size - memory_size);
    // printf("new pointer : %x\n", (uint)new_block);
    new_block->prev = cur_block;
    // printf("nouveau bloc alloué\n");
    new_block->size = memory_size;
    new_block->free = false;
    new_block->magic_number = SAFE_BLOCK_NUMBER;
    new_block->next = cur_block->next;

    cur_block->next = new_block;
    cur_block->size -= memory_size + sizeof(block);

    if(last_block == cur_block) last_block = new_block;
    else new_block->next->prev = new_block;

    // printf("current free list state :\n");
    // print_memory(first_block);
    // printf("\n");
    return new_block;
}

block* find_free_block(block* cur_block, size_t memory_size) {
    if(cur_block == NULL) return NULL;
    if(cur_block->free && cur_block->size >= memory_size)
        return cur_block;
    return find_free_block(cur_block->next, memory_size);
}

uint kmalloc(size_t memory_size) {
    // printf("kmalloc %d\n", memory_size);
    block* free_block = find_free_block(first_block, memory_size);
    // printf("free bloc trouvé %d\n", (uint)free_block);
    if(free_block == NULL) {
        // printf("demande d'un nouveau bloc\n");
        add_block(memory_size);
        free_block = last_block;
    }
    // printf("free list state before malloc :\n");
    // print_memory(first_block);
    // printf("\n");

    if(free_block->size > memory_size + 2 * sizeof(block)) {
        // we cut the block in two
        block* new_block = split_block(free_block, memory_size);
        return (uint)new_block + sizeof(block);
    } else {
        free_block->free = false;
        // printf("current free list state :\n");
        // print_memory(first_block);
        // printf("\n");
        return (uint)free_block + sizeof(block);
    }
}

void merge_with_next_block(block* cur_block) {
    cur_block->size += cur_block->next->size + sizeof(block);
    cur_block->next = cur_block->next->next;
    if(cur_block->next != NULL)
        cur_block->next->prev = cur_block;
    if(cur_block->next == NULL) last_block = cur_block;
}

void kfree(void* ptr) {
    block* free_block = (block*)ptr - 1;
    printf("freeing pointer %d\n", (uint)free_block);
    if(free_block->magic_number != SAFE_BLOCK_NUMBER) {
        // panic : the pointer is wrong
        printf("the pointer given to kfree is wrong\n");
        return;
    }
    free_block->free = true;
    // printf("current free list state before free :\n");
    // print_memory(first_block);
    // printf("\n");
    if(free_block->next != NULL && free_block->next->free == true)
        merge_with_next_block(free_block);
    // printf("current free list state during free :\n");
    // print_memory(first_block);
    // printf("\n");
    if(free_block->prev != NULL && free_block->prev->free == true)
        merge_with_next_block(free_block->prev);
    // printf("current free list state after free :\n");
    // print_memory(first_block);
    // printf("\n");
}

static uint first_pagetable[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
void init_pagemap() {
    for (size_t i = 0; i < TABLE_SIZE; i++)
        page_directory[i] = 0x0002;
    
    page_directory[0] = (uint)&first_pagetable | 3;
    page_directory[TABLE_SIZE-1] = (uint)page_directory | 3;
    for(uint ptx = 0; ptx < TABLE_SIZE; ptx++)
        first_pagetable[ptx] = (ptx << 12) | 3;
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
    gdtr.offset = (uint32_t)gdt;

    setGdtEntry(0, 0, 0, 0, 0); // Null
    setGdtEntry(1, 0, 0xFFFFF, 0x9A, 0xCF); // Kernel code
    setGdtEntry(2, 0, 0xFFFFF, 0x92, 0xCF); // Kernel data
    setGdtEntry(3, 0, 0xFFFFF, 0xFA, 0xCF); // User code
    setGdtEntry(4, 0, 0xFFFFF, 0xF2, 0xCF); // User data

    asm volatile ("lgdt %0" : : "m"(gdtr));
    gdt_flush();
}


/*
 * Setting up the Interrupt Descriptor Table (IDT)
 */

// outb et inb, tiré du `x86.h` de weensyos
static inline void outb(int port, uint8_t data) {
  asm volatile("outb %0,%w1" : : "a"(data), "d"(port));
}
static inline uint8_t inb(int port) {
  uint8_t data;
  asm volatile("inb %w1,%0" : "=a"(data) : "d"(port));
  return data;
}

#define IDT_SIZE 256

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20

#define ICW1_ICW4 0x01          /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02        /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04     /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08         /* Level triggered (edge) mode */
#define ICW1_INIT 0x10          /* Initialization - required! */

#define ICW4_8086 0x01          /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02          /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08     /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM 0x10          /* Special fully nested (not) */

#define CASCADE_IRQ 2

#define KB_DATA 0x60
#define KB_COMMAND 0x64

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
extern void irq_stub_0();

static uint16_t pic_enabled;

static void PIC_mask() {
    outb(PIC1_DATA, ~pic_enabled & 0xFF);
    outb(PIC2_DATA, (~pic_enabled >> 8) & 0xFF);
}

static void PIC_sendEOI(uint8_t irq) {
    if (irq >= 8) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void PIC_disable() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void PIC_remap(uint32_t offset1, uint32_t offset2) {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // start (cascade) sequence
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);

    outb(PIC1_DATA, 1 << CASCADE_IRQ);
    outb(PIC2_DATA, 2);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);
}

uint64_t ticks;
uint32_t intern_freq = 1193182; // Hz
const uint32_t user_freq = 100; // Hz
// intern_freq / user_freq sera retenu, seulement sur 16 bits

void init_timer() {
    uint16_t divisor = intern_freq / user_freq;

    outb(0x43, 0x36); // see osdev PIC page
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}


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

    // init_timer();

    pic_enabled = 0x0000;
    PIC_mask();

    PIC_remap(0x20, 0x28); // IRQs sont les interrupts de 32 à 47

    pic_enabled = 0x0002; // bit0:Timer ; bit1:Keyboard
    PIC_mask();


    for (size_t i = 0; i < IDT_SIZE; i++)
        setIdtEntry(i, isr_stub_0 + (i * 16), 0x8E); // 0b10001110

    for (size_t i = 0; i < 16; i++)
        setIdtEntry(32 + i, irq_stub_0 + (i*16), 0x8E);

    asm volatile ("lidt %0" : : "m"(idtr));
}

typedef struct {
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_num, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) int_regs;

void print_int_regs(int_regs *r) {
    printf("PRINTING INT REGISTERS:\n");
    printf("  ds %x; es %x; fs %x; gs %x\n", r->ds, r->es, r->fs, r->gs);
    printf("  int_num %x; err_code %x\n", r->int_num, r->err_code);
    printf("  eip %x; cs %x; eflags %x; useresp %x; ss %x\n",
            r->eip, r->cs, r->eflags, r->useresp, r->ss);
}

void hexdump(uint32_t longs, uint32_t *ptr) {
    printf("HEXDUMP AT %x :", (uint32_t)ptr);
    for (uint32_t i = 0; i < longs; i++) {
        if (i % 4 == 0) printf("\n  Long %d to %d --", i, i+3);
        printf(" %x", *(ptr + i));
    }
    putchar('\n');
}

int_regs *interrupt_dispatch(int_regs *context) {
    printf("\nINT %d; err_code %x\n", context->int_num, context->err_code);
    switch (context->int_num)
    {
        case 0:
            printf("Division by 0\n");
            break;
        case 6:
            printf("Invalid opcode at eip: %x\n", context->eip);
            break;
        case 13 :
            printf("General Protection Fault");
            if (context->err_code != 0)
                printf(" with segment selector %x", context->err_code);
            putchar('\n');
            break;
        case 14: {
                uint32_t cr2;
                asm volatile("mov %%cr2, %0" : "=r"(cr2));
                printf("Page fault at address %d, err_code %x\n",
                        cr2, context->err_code);
            }
            break;
        default:
            printf("Unhandled interrupt\n");
            break;
    }
    printf("Interrupt handled, halting...\n");
    while (1) {};
    // return context
}

int_regs *irq_dispatch(int_regs *context) {
    switch (context->int_num)
    {
        case 0: // Timer
            ticks++;
            printf("Time ticked\n");
            break;
        case 1: // Keyboard
            (void)(inb(0x60) & 0x7F);
            (void)(inb(0x60) & 0x80);
            printf("Key pressed\n");
            break;
        default:
            printf("IRQ %d : Unhandled irq\n", context->int_num);
            break;
    }
    PIC_sendEOI(context->int_num);
    return context;
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
    asm volatile ("sti");

    // init physical allocator
    init_physical_pageinfo(mbd);

    // init and enable paging
    init_pagemap();
    loadPageDirectory(page_directory);
    enablePaging();

    SAFE_BLOCK_NUMBER = rand();

    printf("kernel: %x - %x\n", start_addr, end_addr);
    printf("pageinfo: %x - %x\n", (uint)pageinfo, (uint)pageinfo + sizeof(pageinfo));
    printf("page_directory: %x\n", (uint)page_directory);
    printf("first_pagetable: %x\n", (uint)first_pagetable);
    printf("brk: %x\n", brk);


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
    printf("quicksort is ok ? %s\n", (ok ? "ok" : "not ok"));

    // printf("pointeur du tableau : %d\n", (uint)a);
    free(a);
}
