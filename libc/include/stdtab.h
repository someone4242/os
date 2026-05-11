#ifndef _STDTAB_H
#define _STDTAB_H

#include <stddef.h>
#include <stdbool.h>

#define DISPLAY_WIDTH 80
#define DISPLAY_HEIGHT 25
#define DISPLAY_SIZE DISPLAY_WIDTH * DISPLAY_HEIGHT

#define NULL_PARSED_CMD (parsed_cmd_t){ 0, NULL }
#define NULL_CMD        (cmd_t){ NULL, NULL }

typedef struct {
    size_t height;
    size_t width;
    size_t size;
    size_t cursor;
    size_t cmd_start;
    bool is_terminal;
    char *data;
} tab_t;

typedef struct {
    int count;
    char **tbl;
} parsed_cmd_t;

typedef struct {
    char *name;
    void (*f)(parsed_cmd_t);
} cmd_t;


/* General Use */
tab_t *tab_create();
void tab_load(tab_t *tab);
void tab_putchar(tab_t *tab, const char ch);
void tab_write(tab_t *tab, const char* data, size_t size);
void tab_writestring(tab_t *tab, const char* data);
void tab_writedecimal(tab_t *tab, const int d);


/* Terminal misc */
void tab_scroll(tab_t *tab, const size_t amount);
void tab_cursor_drop_left(tab_t *tab);
void tab_cursor_drop_down(tab_t *tab);


/* Command Parsing */
parsed_cmd_t tab_parse_command(tab_t *tab);
void free_parsed_command(parsed_cmd_t pcmd);

#endif

