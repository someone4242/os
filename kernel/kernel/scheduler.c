#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/scheduler.h>

// linked list of all the processes
process_t *current_process = NULL;
// linked list of the active processes
process_t* processes_list = NULL;
size_t next_free_pid = 0;

void add_process(process_t* process) {
    process->next = current_process;
    current_process = process;
}

void delete_process(size_t pid) {
    if (current_process != NULL && current_process->pid == pid) {
        process_t* rem = current_process->next;
        free(current_process);
        current_process = rem;
    }
    process_t* process = current_process;
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
    process_t* process = current_process;
    while (process != NULL && process->pid != pid) {
        process = process->next;
    }
    return process;
}