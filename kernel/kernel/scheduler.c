#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/kernel.h>
#include <kernel/allocator.h>
#include <kernel/scheduler.h>

// struct temp_process_t {
//     size_t pid;
//     status_t process_status;
//     int_regs context;
//     uint* root_page_table;
//     char name[NAME_MAX_LEN];
//     struct temp_process_t  *next;
// };

// current process being executed
process_t *current_process = NULL;
// linked list of the active processes
process_t* processes_list = NULL;
size_t next_free_pid = 0;

void add_process(process_t* process) {
    process->next = processes_list;
    processes_list = process;
}

void delete_process(size_t pid) {
    if (processes_list != NULL && processes_list->pid == pid) {
        process_t* rem = processes_list->next;
        free(processes_list);
        processes_list = rem;
    }
    process_t* process = processes_list;
    if (process == NULL) {
        return;
    }
    process_t* ant_process = process;
    process = process->next;
    while (process != NULL && process->pid != pid) {
        process = process->next;
        ant_process = ant_process->next;
    }
    if (process == NULL) {
        return;
    }
    ant_process->next = process->next;
    free(process);
}

process_t* find_process(size_t pid) {
    process_t* process = processes_list;
    while (process != NULL && process->pid != pid) {
        process = process->next;
    }
    return process;
}

int_regs schedule(int_regs context) {
    current_process->context = context;

    if (current_process->next != NULL) {
        current_process = current_process->next;
    } else {
        current_process = processes_list;
    }

    return current_process->context;
}

uint* setup_new_pagedirectory(uintptr_t code_start, uintptr_t code_end) {
    char *old_code = (char*)code_start,
         *new_code = (char*)alloc_virtual_page(code_end - code_start);
    for(uint i = 0; i < code_end - code_start; i++)
        new_code[i] = old_code[i];

    uint* pd = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
    uint* snd_pt = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
    uint* stack_pt = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));

    pd[TABLE_SIZE-1] = (uint)pd;
    pd[TABLE_SIZE-2] = (uint)stack_pt | 3;
    pd[0] = (uint)first_pagetable | 3;
    pd[1] = (uint)snd_pt | 3;

    //on init snd_pt
    for(uint i = 0; i < TABLE_SIZE; i++)
        snd_pt[i] = 2;
    uint nb_code_page = (code_end - code_start + PAGE_SIZE - 1) / PAGE_SIZE;
    for(uint i = 0; i < nb_code_page; i++)
        snd_pt[i] = (code_start + i * PAGE_SIZE) | 3;

    //on init stack_pt
    for(uint i = 0; i < TABLE_SIZE; i++)
        stack_pt[i] = 2;
    for(uint i = TABLE_SIZE-4; i < TABLE_SIZE; i++) {
        uint* new_page = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
        stack_pt[i] = (uint)new_page | 3;
    }
    return pd;
}