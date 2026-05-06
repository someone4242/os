#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/ustar.h>

size_t octascii_to_dec(char *number, int size) {
    size_t res = 0;
    for (int i = 0; i < size; i++) {
        res = res*8 + (number[i] - '0');
    }
    return res;
}

/*
bool is_zeroed(tar_record* current_record) {

    for (int i = 0; i < sizeof(tar_record); i++) {
        if (current_record[i] != '\0') {
            return false;
        }
    }
    return true;
}*/