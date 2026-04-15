#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/interrupt.h>
#include <kernel/tty.h>
#include <kbdriver.h>
#include <kellp.h>

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
                kellp_feedinp(ch);
            break;
        }
        default:
            printf("IRQ %d : Unhandled irq\n", context->int_num);
            break;
    }
    PIC_sendEOI(context->int_num);
    return context;
}

uint8_t get_keyboard_set() {
    outb(0x60, 0xF0);
    outb(0x60, 0x00);
    (void)(inb(0x60) == 0xFA);
    uint8_t ret = inb(0x60);
    if (ret == 0x43) return 1;
    else if (ret == 0x41) return 2;
    else if (ret == 0x3F) return 3;
    else return 0;
}
