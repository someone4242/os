#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/ustar.h>

file_descriptor_t* open(const char *filename) {
    int name_length = strlen(filename);
    if (name_length > FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE) {
        return NULL;
    }
    file_descriptor_t* fd = malloc(sizeof(fd));
    tar_record get_information;
    memset(fd->buffer, '\0', 512);

    char first_name_part[FILE_NAME_MAX_SIZE];
    memset(first_name_part, '\0', FILE_NAME_MAX_SIZE);
    char second_name_part[167];
    memset(second_name_part, '\0', 167);

    if (name_length < FILE_NAME_MAX_SIZE) {
        memcpy(first_name_part, filename, name_length);
    } else {
        memcpy(first_name_part, filename, FILE_NAME_MAX_SIZE);
        memcpy(second_name_part, filename + 100, 
               name_length - FILE_NAME_MAX_SIZE);
    }
    
    int sector_number = 0;
    int nb_zeroes = 0;
    while (nb_zeroes < 2) {
        sector_number++;
        read_sector_pio(sector_number, (uint8_t*) &get_information);
        if (is_zeroed(&get_information)) {
            nb_zeroes++;
            continue;
        }

        nb_zeroes = 0;

        if (IS_A_DIRECTORY == get_information.typeflag) {
            continue;
        }

        if (memcmp(first_name_part, get_information.file_name, 
                   FILE_NAME_MAX_SIZE) == 0
            && memcmp(second_name_part, get_information.file_name_prefix, 
                      FILE_NAME_PREFIX_MAX_SIZE) == 0) {
            break;
        }

        int file_size = octascii_to_dec(get_information.file_size, 
                                        FILE_SIZE_SIZE);
        sector_number += file_size;
    }
    if (nb_zeroes == 2) {
        return NULL;
    }

    fd->file_size = octascii_to_dec(get_information.file_size, 
                                    FILE_SIZE_SIZE);
    fd->next_blck_to_read = sector_number + 1;
    fd->buf_read_pos = 512;

    return fd;
}

void close(file_descriptor_t* fd) {
    free(fd);
}