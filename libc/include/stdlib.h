#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <kernel/ustar.h>

// file system

typedef struct {
    int file_size;
    char buffer[512];
    int buf_read_pos;
    int hd_block;
    int next_blck_to_read;
} file_descriptor_t;


file_descriptor_t* open(const char *filename);
void close(file_descriptor_t* fd);
void read(file_descriptor_t* fd, char* buffer, size_t size);


typedef struct {
    char name[FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE];
    int next_blck_to_read;
} directory_descriptor_t;

typedef struct {
    bool succes; // set to false is no file was read
    bool is_a_directory; // true iff it is a directory
    char name[FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE];
    size_t size;
} file_information;

directory_descriptor_t* opendir(const char* name);
void closedir(directory_descriptor_t* dir);
file_information readdir(directory_descriptor_t* dir);

// -----------

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);

void* malloc(size_t memory);
void free(void* ptr);

void srand(unsigned);
int rand(void);

void qsort(void* ptr, size_t n, size_t size, int (*compare)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif
