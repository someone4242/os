#include <string.h>

int strcount(char c, char* str) {
    int res = 0;
    int i = 0;

    while (str[i] != '\0') {
        if (str[i] == c) {
            res += 1;
        }
        i++;
    }

    return res;
}