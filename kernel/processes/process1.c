#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdtab.h>

#include <syscalls.h>

int main() {
    // terminal_initialize();
    // printf("test de processus\n");
    // tab_t *tab = tab_create();
    //
    char data[DISPLAY_SIZE];
    tab_t tab_strct;
    tab_t *tab = &tab_strct;

    tab->width = DISPLAY_WIDTH;
    tab->height = DISPLAY_HEIGHT;
    tab->size = DISPLAY_SIZE;
    tab->cursor = 0;
    tab->autoscroll = false;
    tab->data = data;

    for (size_t i; i < DISPLAY_SIZE; i++) {
        tab->data[i] = ' ';
    }

    for (size_t i = 0; i < tab->size/2; i++) {
        tab->data[i] = tab->data[tab->size/2 + i];
    }

    tab_writestring(tab, "Ceci est un test");
    tab_cursor_drop_down(tab);
    tab_writestring(tab, "Ceci en est un autre");

    sys_test();
    tab_load(tab);
    while(1) {
    }
    return 0;
}
