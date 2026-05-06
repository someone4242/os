#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <macros.h>

// ----------------------------- STRUCTS ---------------------------------------
typedef struct {
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_num, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) int_regs;
typedef struct {
    uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed)) tss_t;

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
// -----------------------------------------------------------------------------

// public functions
void init_gdt();
void init_idt();
uint8_t get_keyboard_set();
void set_kernel_stack(uint32_t stack);

// Syscalls
void sys_test();

#endif
