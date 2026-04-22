#include <stddef.h>
#include <stdint.h>

#include <macros.h>
#include <kernel/interrupt.h>

// Status of a process
typedef enum {
    READY,
    RUNNING,
    DEAD
} status_t;

// Representing all the processes
struct temp_process_t {
    size_t pid;
    status_t process_status;
    int_regs context;
    uint* root_page_table;
    char name[NAME_MAX_LEN];
    struct temp_process_t  *next;
};

typedef struct temp_process_t process_t;

void add_process(process_t* process);
void delete_process(size_t pid); //does nothing if process absent
process_t* find_process(size_t pid); //return NULL if not found

int_regs schedule(int_regs context);