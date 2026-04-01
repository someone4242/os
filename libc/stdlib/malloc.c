#include <stddef.h>
#if defined(__is_libk)
#include <kernel/allocator.h>
#endif

void* malloc(size_t memory_size) {
    void* ptr = NULL;
    #if defined(__is_libk)
        ptr = (void*)alloc_virtual_page(memory_size);
    #endif
    return ptr;
}