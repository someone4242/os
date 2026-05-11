#include <stdtab.h>
#include <stdlib.h>

void free_parsed_command(parsed_cmd_t pcmd) {
    for (int i = 0; i < pcmd.count; i++) {
        free(pcmd.tbl[i]);
    }
    free(pcmd.tbl);
}
