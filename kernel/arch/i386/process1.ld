ENTRY(_start)

SECTIONS {
    . = 0x400000;

    __ehdr_start = .;       /* début du header ELF — libc en a besoin */

    .text : {
        *(.text .text.*)
    }

    . = ALIGN(4096);
    .rodata : {
        *(.rodata .rodata.*)
    }

    . = ALIGN(4096);

    /* tableaux de constructeurs/destructeurs C++ et __attribute__((constructor)) */
    .init_array : {
        __init_array_start = .;
        *(.init_array .init_array.*)
        __init_array_end = .;
    }

    .fini_array : {
        __fini_array_start = .;
        *(.fini_array .fini_array.*)
        __fini_array_end = .;
    }

    .preinit_array : {
        __preinit_array_start = .;
        *(.preinit_array)
        __preinit_array_end = .;
    }

    . = ALIGN(4096);
    .tdata : {              /* thread-local data initialisé */
        *(.tdata .tdata.*)
    }

    .tbss : {               /* thread-local data non-initialisé */
        *(.tbss .tbss.*)
    }

    . = ALIGN(4096);
    .data : {
        *(.data .data.*)
    }

    . = ALIGN(4096);
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }

    . = ALIGN(4096);
    _end = .;               /* fin du programme — utilisé par malloc */
}