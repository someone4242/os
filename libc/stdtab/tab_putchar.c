#include <stdtab.h>

void tab_putchar(tab_t *tab, const char ch) {
    tab->data[tab->cursor] = ch;
    if (tab->cursor++ == tab->size) {
        if (tab->autoscroll) tab_scroll(tab, 1);
        else tab->cursor = 0;
    }
}

