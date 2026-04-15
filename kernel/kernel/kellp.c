/* Kernel Shell Prototype - C */

#include <kellp.h>

#include <string.h>
#include <kernel/tty.h>

#define TAB_WIDTH 80
#define TAB_HEIGHT 25
#define TAB_SIZE TAB_WIDTH * TAB_HEIGHT
static char tab[TAB_SIZE];
static size_t cursor;
static size_t cmd_start;


/* Utilitaries */

static void tab_scroll(const size_t amount) {
    size_t i;
    for (i = 0; i < (TAB_HEIGHT - amount) * TAB_WIDTH; i++) {
        const uint16_t c = tab[amount * TAB_WIDTH + i];
        tab[i] = c;
    }
    for (i = (TAB_HEIGHT-amount)*TAB_WIDTH; i < TAB_HEIGHT * TAB_WIDTH; i++) {
        tab[i] = ' ';
    }
}

static void drop_cursor() {
    while (cmd_start < cursor && tab[cursor - 1] == 0x00) cursor--;
}


/* Inspired from TTY */

void kellp_putchar(char ch) {
    tab[cursor] = ch;
    if (cursor++ == TAB_SIZE) {
        tab_scroll(1);
        cursor -= TAB_WIDTH;
    }
}

void kellp_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        kellp_putchar(data[i]);
}

void kellp_writestring(const char* data) {
    kellp_write(data, strlen(data));
}




/* Interface */

void init_kellp() {
    for (size_t i = 0; i < TAB_SIZE; i++) {
        tab[i] = 0x00;
    }

    tab[0] = '>'; tab[1] = '>'; tab [2] = ' ';
    cmd_start = 3;
    cursor = 3;

    terminal_update(tab, cursor);
}

void kellp_feedinp(char ch) {
    switch (ch)
    {
        case '\n': // A changer : correspondrait au Shift + Enter
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH + 1);
            if (cursor >= TAB_SIZE) {
                tab_scroll(1);
                cursor -= TAB_WIDTH;
            }
            break;

        case 'N': // temporaire
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH + 1);
            if (cursor >= TAB_SIZE) {
                tab_scroll(1);
                cursor -= TAB_WIDTH;
            }
            kellp_writestring(">> ");
            cmd_start = cursor;
            break;

        case '\r': // input possible même ?
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH);
            break;

        case '\b':
            if (cursor == cmd_start) break;
            cursor--;
            tab[cursor] = 0x00;
            drop_cursor();
            break;

        default:
            kellp_putchar(ch);
            break;
    }

    terminal_update(tab, cursor);
}

