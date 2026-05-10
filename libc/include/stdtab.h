#ifndef _STDTAB_H
#define _STDTAB_H

#include <stddef.h>
#include <stdbool.h>

#define DISPLAY_WIDTH 80
#define DISPLAY_HEIGHT 25
#define DISPLAY_SIZE DISPLAY_WIDTH * DISPLAY_HEIGHT

typedef struct {
    size_t height;
    size_t width;
    size_t size;
    size_t cursor;
    size_t cmd_start; // only valid if autoscroll is true
    bool autoscroll; // 0 : cursor loops back | 1 : autoscrolls
    char *data;
} tab_t;

/* General Use */
tab_t *tab_create();
void tab_load(tab_t *tab);
void tab_putchar(tab_t *tab, const char ch);
void tab_write(tab_t *tab, const char* data, size_t size);
void tab_writestring(tab_t *tab, const char* data);


/* Terminal misc */
void tab_scroll(tab_t *tab, const size_t amount);
void tab_cursor_drop_left(tab_t *tab);
void tab_cursor_drop_down(tab_t *tab);

#endif

