# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	# Call the global constructors.
	push %eax
	push %ebx
	call _init

	# Transfer control to the main kernel.
	call kernel_main

	# Hang if kernel_main unexpectedly returns.
	cli
1:	hlt
	jmp 1b
.size _start, . - _start


.text
.globl loadPageDirectory
loadPageDirectory:
    push %ebp
    movl %esp, %ebp
    movl 8(%esp), %eax
    movl %eax, %cr3
    movl %ebp, %esp
    popl %ebp
    ret


.text
.globl enablePaging
enablePaging:
    push %ebp
    movl %esp, %ebp
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    movl %ebp, %esp
    popl %ebp
    ret



// GDT, IDT et ISRs

.text
.globl gdt_flush
gdt_flush:
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss
    jmp .flush
.flush:
    ret


interrupt_stub:
    // A faire : push les registres d'environnement
    // puis modifier la gestion d'arguments de `interrupt_dispatch`

    call interrupt_dispatch

    // A faire : pop les registres d'environnement

    add $16, %esp       // retire int_num et err_code de la stack
    iret



.macro isr_err_stub n
.globl isr_stub_\n
.align 16
isr_stub_\n :
    pushl $\n           // int_num
    jmp interrupt_stub
.endm

.macro isr_no_err_stub n
.globl isr_stub_\n
.align 16
isr_stub_\n :
    pushl $0            // err_code factice
    pushl $\n           // int_num
    jmp interrupt_stub
.endm

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

