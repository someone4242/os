#include <stddef.h>
#if defined(__is_libk)
#include <kernel/allocator.h>
#endif

void* malloc(size_t memory_size) {
    void* ptr = NULL;
    #if defined(__is_libk)
        ptr = (void*)kmalloc(memory_size);
    #endif
    return ptr;
}

void free(void* ptr) {
    #if defined(__is_libk)
        kfree(ptr);
    #endif
}