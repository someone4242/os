#ifndef _KERNEL_H 
#define _KERNEL_H 

#include <stddef.h>
#include <stdint.h>
#include <kernel/scheduler.h>
#include <macros.h>

extern uint page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
extern uint first_pagetable[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
void init_pagemap();

#endif