#include <stdtab.h>
#include <string.h>

void tab_writestring(tab_t *tab, const char* data) {
    tab_write(tab, data, strlen(data));
}
