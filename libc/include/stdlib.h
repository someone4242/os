#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

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
