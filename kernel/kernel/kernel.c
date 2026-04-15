#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "macros.h"
#include "kernel/allocator.h"
#include "kernel/tty.h"
#include "kernel/multiboot.h"
#include "kernel/kernel.h"
#include "kbdriver.h"

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
        {
            uint8_t scancode = inb(0x60);
            uint8_t keycode = kb_scan_to_key(scancode);
            char ch = kb_key_to_ascii(keycode);
            if (ch != 0)
                putchar(ch);
            break;
        }
        default:
            printf("IRQ %d : Unhandled irq\n", context->int_num);
            break;
    }
    PIC_sendEOI(context->int_num);
    return context;
}

static uint8_t get_keyboard_set() {
    outb(0x60, 0xF0);
    outb(0x60, 0x00);
    (void)(inb(0x60) == 0xFA);
    uint8_t ret = inb(0x60);
    if (ret == 0x43) return 1;
    else if (ret == 0x41) return 2;
    else if (ret == 0x3F) return 3;
    else return 0;
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
