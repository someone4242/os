#include <stddef.h>
#include <stdint.h>

#include <macros.h>

static uint page_directory[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint first_pagetable[TABLE_SIZE] __attribute__((aligned(PAGE_SIZE)));
void init_pagemap();