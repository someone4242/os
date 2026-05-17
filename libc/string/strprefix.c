#include <string.h>
#include <stdbool.h>

bool strprefix(char* pref, char* str) {
    int i = 0;
    while (pref[i] != '\0' && str[i] != '\0') {
        if (pref[i] != str[i]) {
            return false;
        }
        i++;
    }
    return pref[i] == '\0';
}