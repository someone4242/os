#include <stddef.h>
#include <kernel/allocator.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

static const uint SAFE_BLOCK_NUMBER = 0x13a931a;

static block *first_block = NULL, *last_block = NULL;

#if !defined(__is_libk)
uint syscall_alloc_virtual_page(size_t memory_size) {
    uint result;
    asm volatile("int $0x34"
                : "=a"(result)
                : "b"(memory_size)
                : "memory", "cc");
    return result;
}
#endif

block* get_new_block(size_t memory_size) {
    block* ptr;
    #if defined(__is_libk)
        ptr = (block*)alloc_virtual_page(memory_size + sizeof(block));
    #else 
        ptr = (block*)syscall_alloc_virtual_page(memory_size + sizeof(block));
    #endif
    ptr->size = memory_size;
    ptr->free = true;
    ptr->next = NULL;
    ptr->prev = NULL;
    ptr->magic_number = SAFE_BLOCK_NUMBER;
    return ptr;
}

void push_back(block* cur_block) {
    if(first_block == NULL) {
        last_block = first_block = cur_block;
    } else {
        last_block->next = cur_block;
        cur_block->prev = last_block;
        last_block = cur_block;
    }
}

void add_block(size_t memory_size) {
    size_t mem_alloc = memory_size + (sizeof(int) * (1 << 16));
    block* ptr = get_new_block(mem_alloc);

    push_back(ptr);
}

block* split_block(block* cur_block, size_t memory_size) {
    block* new_block = (block*)((uint)cur_block + cur_block->size - memory_size);
    new_block->prev = cur_block;
    new_block->size = memory_size;
    new_block->free = false;
    new_block->magic_number = SAFE_BLOCK_NUMBER;
    new_block->next = cur_block->next;

    cur_block->next = new_block;
    cur_block->size -= memory_size + sizeof(block);

    if(last_block == cur_block) last_block = new_block;
    else new_block->next->prev = new_block;

    return new_block;
}

block* find_free_block(block* cur_block, size_t memory_size) {
    if(cur_block == NULL) return NULL;
    if(cur_block->free && cur_block->size >= memory_size)
        return cur_block;
    return find_free_block(cur_block->next, memory_size);
}

uint malloc(size_t memory_size) {
    block* free_block = find_free_block(first_block, memory_size);
    if(free_block == NULL) {
        add_block(memory_size);
        free_block = last_block;
    }

    if(free_block->size > memory_size + 2 * sizeof(block)) {
        // we cut the block in two
        block* new_block = split_block(free_block, memory_size);
        return (uint)new_block + sizeof(block);
    } else {
        free_block->free = false;
        return (uint)free_block + sizeof(block);
    }
}

void merge_with_next_block(block* cur_block) {
    cur_block->size += cur_block->next->size + sizeof(block);
    cur_block->next = cur_block->next->next;
    if(cur_block->next != NULL)
        cur_block->next->prev = cur_block;
    if(cur_block->next == NULL) last_block = cur_block;
}

void free(void* ptr) {
    block* free_block = (block*)ptr - 1;
    printf("freeing pointer %d\n", (uint)free_block);
    if(free_block->magic_number != SAFE_BLOCK_NUMBER) {
        // panic : the pointer is wrong
        printf("the pointer given to kfree is wrong\n");
        return;
    }
    free_block->free = true;

    if(free_block->next != NULL && free_block->next->free == true)
        merge_with_next_block(free_block);
    if(free_block->prev != NULL && free_block->prev->free == true)
        merge_with_next_block(free_block->prev);
}
