#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
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

void copy_regs_to_save(int_regs* context, process_t* process) {
    memcpy(&process->context, context, sizeof(int_regs));
}

void load_save_regs(int_regs* context, process_t* process) {
    memcpy(context, &process->context, sizeof(int_regs));
}

int_regs* schedule(int_regs* context) {
    if (processes_list == NULL) {
        return context;
    }
    if (current_process == NULL) {
        current_process = processes_list;
    } else {
        copy_regs_to_save(context, current_process);
    }
    current_process->process_status = READY;

    while (true) {
        if (current_process->next != NULL) {
            current_process = current_process->next;
        } else {
            current_process = processes_list;
        }

        if (current_process != NULL && current_process->process_status == DEAD) {
            // We need to delete dead processes
            delete_process(current_process->pid);
        } else {
            current_process->process_status = RUNNING;
            break;
        }
    }
    load_save_regs(context, current_process);
    return context;//&current_process->context;
}

uint* setup_new_pagedirectory(uintptr_t code_start, uintptr_t code_end) {
    char *old_code = (char*)code_start,
         *new_code = (char*)alloc_virtual_page(code_end - code_start);
    memcpy(new_code, old_code, code_end - code_start);
    printf("adresse de new code : %x, longueur : %x\n", (uint)new_code, code_end - code_start);

    uint* pd = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
    memcpy(pd, page_directory, TABLE_SIZE * sizeof(uint));
    uint* third_pt = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
    uint* stack_pt = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));

    pd[TABLE_SIZE-1] = (uint)pd | 3;
    pd[TABLE_SIZE-2] = (uint)virt_to_phys((uint)stack_pt) | 7;
    pd[0] = (uint)virt_to_phys((uint)first_pagetable) | 7;
    pd[1] = (uint)virt_to_phys((uint)second_pagetable) | 3;
    pd[2] = (uint)virt_to_phys((uint)third_pt) | 7;
    first_pagetable[0xB8] = 0xB8000 | 7; 

    //on init snd_pt
    for(uint i = 0; i < TABLE_SIZE; i++)
        third_pt[i] = 6;
    uint nb_code_page = (code_end - code_start + PAGE_SIZE - 1) / PAGE_SIZE;
    printf("nb code pages : %d\n", nb_code_page);
    for(uint i = 0; i < nb_code_page; i++)
        third_pt[i] = ((uint)virt_to_phys((uint)new_code + i * PAGE_SIZE)) | 7;

    //on init stack_pt
    print_brk();
    printf("stack_pt : %d\n", (uint)stack_pt);
    for(uint i = 0; i < TABLE_SIZE; i++)
        stack_pt[i] = 6;
    for(uint i = TABLE_SIZE-4; i < TABLE_SIZE; i++) {
        uint* new_page = (uint*)alloc_virtual_page(TABLE_SIZE * sizeof(uint));
        stack_pt[i] = (uint)virt_to_phys((uint)new_page) | 7;
    }
    return pd;
}

process_t* create_process(char* name, uintptr_t code_start, uintptr_t code_end, void* arg) {
    process_t* process = malloc(sizeof(process_t));

    process->root_page_table = setup_new_pagedirectory(code_start, code_end);
    memcpy(process->name, name, NAME_MAX_LEN);
    process->pid = next_free_pid++;
    process->process_status = READY;
    process->context.ss = USER_DS;
    process->context.esp = 0;
    process->context.eflags = 0x202;
    process->context.cs = USER_CS;
    process->context.eip = 0x800000;
    process->context.edi = (uint32_t)arg;

    // initialisation du reste
    process->context.gs = USER_DS;
    process->context.fs = USER_DS;
    process->context.es = USER_DS;
    process->context.ds = USER_DS;
    process->context.esi = 0;
    process->context.eax = 0;
    process->context.ebx = 0;
    process->context.ecx = 0;
    process->context.edx = 0;
    process->context.int_num = 0;
    process->context.err_code = 0;
    process->context.ebp = 0xFFBFFF00;
    process->context.useresp = 0xFFBFFF00;
    process->next = NULL;

    add_process(process);
    
    return process;
}

