#include <stdtab.h>

void tab_write(tab_t *tab, const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        tab_putchar(tab, data[i]);
}
