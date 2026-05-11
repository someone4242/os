#include <stdtab.h>

void tab_scroll(tab_t *tab, const size_t amount) {
    size_t i;
    for (i = 0; i < (tab->height - amount) * tab->width; i++) {
        const char ch = tab->data[amount * tab->width + i];
        tab->data[i] = ch;
    }
    for (i = (tab->height - amount) * tab->width; i < tab->size; i++) {
        tab->data[i] = ' ';
    }

    if (tab->cursor < amount * tab->width) {
        tab->cursor = 0;
        tab->cmd_start = 0;
    } else {
        tab->cursor -= amount * tab->width;
        tab->cmd_start -= amount * tab->width;
    }
}

void tab_cursor_drop_left(tab_t *tab) {
    while (tab->data[tab->cursor - 1] == 0x00) tab->cursor--;
}

void tab_cursor_drop_down(tab_t *tab) {
    tab->cursor = tab->width * (tab->cursor / tab->width + 1);
    if  (tab->cursor >= tab->size) {
        if (tab->is_terminal) tab_scroll(tab, 1);
        else tab->cursor = 0;
    }
}
