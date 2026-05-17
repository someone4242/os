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
    
    tar_record get_information;
    

    char first_name_part[FILE_NAME_MAX_SIZE];
    memset(first_name_part, '\0', FILE_NAME_MAX_SIZE);
    char second_name_part[FILE_NAME_PREFIX_MAX_SIZE];
    memset(second_name_part, '\0', FILE_NAME_PREFIX_MAX_SIZE);

    if (name_length < FILE_NAME_MAX_SIZE) {
        memcpy(first_name_part, filename, name_length);
    } else {
        memcpy(first_name_part, filename, FILE_NAME_MAX_SIZE);
        memcpy(second_name_part, filename + FILE_NAME_MAX_SIZE, 
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

    file_descriptor_t* fd = malloc(sizeof(fd));
    memset(fd->buffer, '\0', 512);
    fd->file_size = octascii_to_dec(get_information.file_size, 
                                    FILE_SIZE_SIZE);
    fd->next_blck_to_read = sector_number + 1;
    fd->hd_block = sector_number;
    fd->buf_read_pos = 512;

    return fd;
}

void close(file_descriptor_t* fd) {
    free(fd);
}

void read(file_descriptor_t* fd, char* buffer, size_t size) {
    int i = 0;
    //printf("%d, %d, %d\n", fd->next_blck_to_read, fd->hd_block, fd->file_size);
    
    while (67) {
        while (fd->buf_read_pos < 512 && size > 0) {
            size--;
            buffer[i] = fd->buffer[fd->buf_read_pos];
            fd->buf_read_pos++;
            i++;
        }

        if (size == 0) return;

        if (fd->next_blck_to_read > fd->hd_block + fd->file_size) {
            break;
        }
        
        read_sector_pio(fd->next_blck_to_read, (uint8_t*)fd->buffer);
        //printf("%s\n", fd->buffer);
        fd->buf_read_pos = 0;
        fd->next_blck_to_read++;
    }
    while (size > 0) {
        buffer[i] = '\0';
        i++;
        size--;
    }
}


directory_descriptor_t* opendir(const char* name) {
    int name_length = strlen(name);
    if (name_length > FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE) {
        return NULL;
    }

    tar_record get_information;

    char first_name_part[FILE_NAME_MAX_SIZE];
    memset(first_name_part, '\0', FILE_NAME_MAX_SIZE);
    char second_name_part[FILE_NAME_PREFIX_MAX_SIZE];
    memset(second_name_part, '\0', FILE_NAME_PREFIX_MAX_SIZE);

    if (name_length < FILE_NAME_MAX_SIZE) {
        memcpy(first_name_part, name, name_length);
    } else {
        memcpy(first_name_part, name, FILE_NAME_MAX_SIZE);
        memcpy(second_name_part, name + FILE_NAME_MAX_SIZE, 
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

        if (IS_A_REGULAR_FILE == get_information.typeflag) {
            int file_size = octascii_to_dec(get_information.file_size, 
                                        FILE_SIZE_SIZE);
            sector_number += file_size;
            continue;
        }

        if (memcmp(first_name_part, get_information.file_name, 
                   FILE_NAME_MAX_SIZE) == 0
            && memcmp(second_name_part, get_information.file_name_prefix, 
                      FILE_NAME_PREFIX_MAX_SIZE) == 0) {
            break;
        }

        sector_number++;
    }
    if (nb_zeroes == 2) {
        return NULL;
    }

    directory_descriptor_t* dir = malloc(sizeof(directory_descriptor_t));
    memset(dir->name, '\0', FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE);
    memcpy(dir->name, name, FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE);
    dir->next_blck_to_read = 1;

    return dir;
}

void closedir(directory_descriptor_t* dir) {
    free(dir);
}

file_information readdir(directory_descriptor_t* dir) {
    file_information res;
    res.succes = false;
    res.is_a_directory = false;
    res.size = 0;
    memset(res.name, '\0', FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE);
    //printf("Hmm zerator, %d\n", dir->next_blck_to_read);
    if (dir == NULL) return res;
    if (dir->next_blck_to_read == -1) return res;
    
    tar_record get_information;
    read_sector_pio(dir->next_blck_to_read, (uint8_t*) &get_information);
    if (is_zeroed(&get_information)) {
        dir->next_blck_to_read = -1;
        return res;
    }
    
    char complete_name[FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE];
    memset(complete_name, '\0', 
            FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE);
    memcpy(complete_name, get_information.file_name, FILE_NAME_MAX_SIZE);
    memcpy(complete_name + FILE_NAME_MAX_SIZE, 
            get_information.file_name_prefix,
            FILE_NAME_PREFIX_MAX_SIZE);

    if (IS_A_REGULAR_FILE == get_information.typeflag) {

        res.is_a_directory = false;
        res.size = octascii_to_dec(get_information.file_size, 12)*512;

        int file_size = octascii_to_dec(get_information.file_size, 
                                    FILE_SIZE_SIZE);
        dir->next_blck_to_read += file_size + 2;
    } else {
        res.is_a_directory = true;
        dir->next_blck_to_read += 2;
    }

    
    if (strprefix(dir->name, complete_name) && 
        (!(strprefix(complete_name, dir->name))) &&
        strcount('/', dir->name) + (get_information.typeflag == IS_A_DIRECTORY)
         == strcount('/', complete_name)) {
        memcpy(res.name, complete_name, 
                FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE);
        res.succes = true;
        return res;
    }
    return readdir(dir);
}