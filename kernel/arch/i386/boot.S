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
    orl $0x80000001, %eax
    movl %eax, %cr0
    movl %ebp, %esp
    popl %ebp
    ret



// GDT, IDT, ISRs et IRQs

.text
.globl gdt_flush
gdt_flush:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush
.flush:
    ret


interrupt_stub:
    pusha

    pushw %ds
    pushw %es
    pushw %fs
    pushw %gs

    pushl %esp
    call interrupt_dispatch
    movl %eax, %esp         // se place à la stack renvoyée

    popw %gs
    popw %fs
    popw %es
    popw %ds

    popa
    add $8, %esp            // retire int_num et err_code de la stack
    sti
    iret

irq_stub:
    pusha

    pushw %ds
    pushw %es
    pushw %fs
    pushw %gs

    pushl %esp
    call irq_dispatch
    movl %eax, %esp         // se place à la stack renvoyée

    popw %gs
    popw %fs
    popw %es
    popw %ds

    popa
    add $8, %esp            // retire int_num et err_code de la stack
    sti
    iret



.macro isr_err_stub n
.globl isr_stub_\n
.align 16
isr_stub_\n :
    cli
    pushl $\n           // int_num
    jmp interrupt_stub
.endm

.macro isr_no_err_stub n
.globl isr_stub_\n
.align 16
isr_stub_\n :
    cli
    pushl $0            // err_code factice
    pushl $\n           // int_num
    jmp interrupt_stub
.endm

.rept 16
.globl irq_stub_\+
.align 16
irq_stub_\+ :
    cli
    pushl $0
    pushl $\+
    jmp irq_stub
.endr


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


