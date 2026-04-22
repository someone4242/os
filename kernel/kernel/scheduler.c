#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/scheduler.h>

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
    process_t* prempted_process;
    current_process->context = context;
    current_process->process_status = READY;

    while (true) {
        process_t *prev_process = current_process;
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
    return current_process->context;
}


process_t* create_process(char* name, void(*function)(void*), void* arg) {
    process_t* process = malloc(sizeof(process_t));

    strncpy(process->name, name, NAME_MAX_LEN);
    process->pid = next_free_pid++;
    process->process_status = READY;
    process->context.ss = KERNEL_DS;
    process->context.esp = alloc_stack();
    process->context.eflags = 0x202;
    process->context.cs = KERNEL_CS;
    process->context.eip = (uint64_t)function;
    process->context.edi = (uint64_t)arg;
    process->context.ebp = 0;

    add_process(process);

    return process;
}