#ifndef _ALLOCATOR
#define _ALLOCATOR

#include <stddef.h>
#include <stdint.h>

uint32_t alloc_virtual_page(size_t memory_size);
uint32_t kmalloc(size_t memory_size);
void kfree(void* ptr);

#endif