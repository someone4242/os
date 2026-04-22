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
    current_process->context = context;

    if (current_process->next != NULL) {
        current_process = current_process->next;
    } else {
        current_process = processes_list;
    }

    return current_process->context;
}
