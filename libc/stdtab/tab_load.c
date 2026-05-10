#include <stdtab.h>
#include <syscalls.h>


void tab_load(tab_t *tab) {
    sys_test();
    sys_termload((void *)tab);
}
