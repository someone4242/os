#include <stdtab.h>

void tab_writedecimal(tab_t *tab, const int d_in) {
    int d = d_in;
    bool has_a_sign = d < 0;
    if (d < 0) d = - d;
    char buffer[16] = {'\0'};
    int i = 0;
    if (d == 0) buffer[i++] = '0';
    while (d > 0) {
        char c = (d % 10) + '0';
        d = d / 10;
        buffer[i++] = c;
    }
    if (has_a_sign) {
        buffer[i++] = '-';
    }
    buffer[i] = '\0';
    size_t len = i;
    for (int j = 0; 2*j < i; j++) {
        char rem = buffer[j];
        int a = i - 1 - j;
        buffer[j] = buffer[a];
        buffer[a] = rem;
    }
    tab_write(tab, buffer, len);
}
