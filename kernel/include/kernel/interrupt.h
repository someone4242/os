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