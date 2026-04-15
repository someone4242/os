#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "macros.h"
#include "kernel/multiboot.h"

typedef struct physical_page_info {
    // 0 -> libre
    // 1 -> reservé
    uint8_t flags;
} physical_page_info;

typedef struct block {
    size_t size;
    bool free;
    uint magic_number;
    struct block* next;
    struct block* prev;
} block;

extern uint _kernel_start;
extern uint _kernel_end;
#define start_addr ((uint)&_kernel_start)
#define end_addr ((uint)&_kernel_end)

extern void loadPageDirectory(uint *);
extern void enablePaging();

static physical_page_info pageinfo[NB_PAGE] __attribute__((aligned(PAGE_SIZE)));

//toute les fonctions peut être a enlever
void init_physical_pageinfo(multiboot_info_t* mbd);
bool is_free_physical_page(uint pageid);
uintptr_t alloc_physical_page();
void virtual_memory_map(uint virtual_addr, uint physical_addr, uint8_t perm);
uint alloc_virtual_page(size_t memory_size);
uint kmalloc(size_t memory_size);
void kfree(void* ptr);


//fonctions publiques
uint brk_public_get();
void brk_public_edit(uint n);
uint SAFE_BLOCK_NUMBER_public_get();
void SAFE_BLOCK_NUMBER_public_edit(uint n);