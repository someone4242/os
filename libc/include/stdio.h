#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>
#include <stdbool.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif


int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

// Usage : `fprintf(print_function, format, args...)`
// where `print_function` takes a string and its length
int fprintf(bool (*print)(const char*, size_t), const char* restrict, ...);


#ifdef __cplusplus
}
#endif

#endif
