#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/allocator.h>
#include <kernel/tty.h>
#include <kernel/multiboot.h>


// we take the infos given by grub about memory map and fill in the page info table
void init_physical_pageinfo(multiboot_info_t* mbd) {
    for(size_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        for(uint addr = mmmt->addr_low; addr < mmmt->addr_low+mmmt->len_low; addr += PAGE_SIZE) {
            pageinfo[PAGEID(addr)].flags = (mmmt->type == 1 ? FREE : RESERVED);
        }
    }
    for(uint addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
        pageinfo[PAGEID(addr)].flags = 1;
    }
}

bool is_free_physical_page(uint pageid) {
    return pageinfo[pageid].flags == FREE;
}

uint pageid = NB_PAGE;
static uint* const recursive_pd = (uint*)0xFFFFF000;
static uint* const recursive_pt = (uint*)0xFFC00000;

uintptr_t alloc_physical_page() {
    if(pageid == NB_PAGE) pageid = PAGEID(end_addr);
    uint cnt = 0;
    while(cnt < NB_PAGE && !is_free_physical_page(pageid)) {
        pageid++, cnt++;
        if(pageid == PAGEID((uint)recursive_pt))
            pageid = PAGEID(end_addr);
    }
    if(cnt == NB_PAGE) {
        // panic : memory limit exceeded
        printf("memory limit exceeded\n");
        return 0;
    }

    pageinfo[pageid].flags = RESERVED;
    return (uintptr_t)(pageid * PAGE_SIZE);
}



void virtual_memory_map(uint virtual_addr, uint physical_addr, uint8_t perm) {
    uint pdx = virtual_addr >> 22;

    if((recursive_pd[pdx] & 1) == 0) {
        // the page isn't present
        recursive_pd[pdx] = alloc_physical_page() | perm | 3;

        // we clean up the table
        uint* table_virt = (uint*)(0xFFC00000 + (pdx << 12));
        for(int i = 0; i < 1024; i++) table_virt[i] = 0;
    }

    recursive_pt[virtual_addr >> 12] = physical_addr | perm | 3;
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
}

// we allocate the virtual addresses incrementally
uint brk = TABLE_SIZE * PAGE_SIZE;
uint alloc_virtual_page(size_t memory_size) {
    uint nb_pages = ((uint)memory_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint virtual_addr = brk;
    printf("virtual allocation : from %d", brk);
    brk += PAGE_SIZE * nb_pages;
    printf(" to %d\n", brk);
    for(uint i_page = 0; i_page < nb_pages; i_page++) {
        uint physical_addr = (uint)alloc_physical_page();

        virtual_memory_map(virtual_addr + i_page * PAGE_SIZE, physical_addr, DEFAULT);
    }
    return virtual_addr;
}


static uint SAFE_BLOCK_NUMBER = 0x15366593;
static block* first_block = NULL;
static block* last_block = NULL;

void print_memory(block* bloc) {
    if(bloc == NULL) return;
    printf("ptr : %d, size : %d, free : %s, next : %d, prev : %d\n", (uint)bloc, bloc->size, (bloc->free ? "yes" : "no"), (uint)bloc->next, (uint)bloc->prev);
    print_memory(bloc->next);
}

block* get_new_block(size_t memory_size) {
    block* ptr = (block*)alloc_virtual_page(memory_size + sizeof(block));
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
    size_t mem_alloc = memory_size + (sizeof(int) * (1 << 20));
    block* ptr = get_new_block(mem_alloc);

    // printf("first block %d, last block %d\n", (uint)first_block, (uint)last_block);
    push_back(ptr);
}

block* split_block(block* cur_block, size_t memory_size) {
    // printf("trying to cut in half\n");
    block* new_block = (block*)((uint)cur_block + cur_block->size - memory_size);
    // printf("new pointer : %x\n", (uint)new_block);
    new_block->prev = cur_block;
    // printf("nouveau bloc alloué\n");
    new_block->size = memory_size;
    new_block->free = false;
    new_block->magic_number = SAFE_BLOCK_NUMBER;
    new_block->next = cur_block->next;

    cur_block->next = new_block;
    cur_block->size -= memory_size + sizeof(block);

    if(last_block == cur_block) last_block = new_block;
    else new_block->next->prev = new_block;

    // printf("current free list state :\n");
    // print_memory(first_block);
    // printf("\n");
    return new_block;
}

block* find_free_block(block* cur_block, size_t memory_size) {
    if(cur_block == NULL) return NULL;
    if(cur_block->free && cur_block->size >= memory_size)
        return cur_block;
    return find_free_block(cur_block->next, memory_size);
}

uint kmalloc(size_t memory_size) {
    // printf("kmalloc %d\n", memory_size);
    block* free_block = find_free_block(first_block, memory_size);
    // printf("free bloc trouvé %d\n", (uint)free_block);
    if(free_block == NULL) {
        // printf("demande d'un nouveau bloc\n");
        add_block(memory_size);
        free_block = last_block;
    }
    // printf("free list state before malloc :\n");
    // print_memory(first_block);
    // printf("\n");

    if(free_block->size > memory_size + 2 * sizeof(block)) {
        // we cut the block in two
        block* new_block = split_block(free_block, memory_size);
        return (uint)new_block + sizeof(block);
    } else {
        free_block->free = false;
        // printf("current free list state :\n");
        // print_memory(first_block);
        // printf("\n");
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

void kfree(void* ptr) {
    block* free_block = (block*)ptr - 1;
    printf("freeing pointer %d\n", (uint)free_block);
    if(free_block->magic_number != SAFE_BLOCK_NUMBER) {
        // panic : the pointer is wrong
        printf("the pointer given to kfree is wrong\n");
        return;
    }
    free_block->free = true;
    // printf("current free list state before free :\n");
    // print_memory(first_block);
    // printf("\n");
    if(free_block->next != NULL && free_block->next->free == true)
        merge_with_next_block(free_block);
    // printf("current free list state during free :\n");
    // print_memory(first_block);
    // printf("\n");
    if(free_block->prev != NULL && free_block->prev->free == true)
        merge_with_next_block(free_block->prev);
    // printf("current free list state after free :\n");
    // print_memory(first_block);
    // printf("\n");
}




//functions to share variables
uint brk_public_get() {
    return brk;
}

void brk_public_edit(uint n) {
    brk = n;
    return;
}

uint SAFE_BLOCK_NUMBER_public_get() {
    return brk;
}

void SAFE_BLOCK_NUMBER_public_edit(uint n) {
    brk = n;
    return;
}