#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

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
