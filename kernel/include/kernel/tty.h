#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <macros.h>

void terminal_initialize(void);
void terminal_update(const char* data, size_t lim_in);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

#endif
