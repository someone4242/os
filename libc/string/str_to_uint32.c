#include <string.h>

uint32_t str_to_uint32(char *s) {
    size_t i = 0;
    uint32_t k = 0;
    while (s[i]) {
        if ('0' <= s[i] && s[i] <= '9') {
            k *= 10;
            k += (uint32_t)(s[i] - '0');
        }
        i++;
    }
    return k;
}
