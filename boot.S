/* boot.S - Minimal multiboot entry point for x86 */

/* Multiboot header constants */
.set MULTIBOOT_MAGIC,        0x1BADB002  /* Magic number for multiboot */
.set MULTIBOOT_FLAGS,        0x00000003  /* Flags: align modules + memory info */
.set MULTIBOOT_CHECKSUM,     -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

/* Multiboot header - must be in first 8KB of kernel */
.section .multiboot, "a", @progbits
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

/* PVH ELF note for QEMU 10.x - tells QEMU this is a valid 32-bit kernel */
.section .note.Xen, "a", @note
.align 4
.long 4                    /* name size */
.long 4                    /* desc size */  
.long 18                   /* XEN_ELFNOTE_PHYS32_ENTRY */
.ascii "Xen\0"            /* name */
.long _start               /* entry point address */

/* Reserve stack space (64KB should be good?) */
.section .bss
.align 16
stack_bottom:
.skip 16384  /* 64KB stack */
stack_top:

/* The actual entry point */
.section .text
.code32
.global _start
.type _start, @function

_start:
    cli              /* Disable interrupts */
    /* Set up stack pointer */
    mov $stack_top, %esp

    /* First, call platform_init_args with multiboot info */
    push %ebx   /* Second argument: multiboot info */
    push %eax   /* First argument: magic number */
    call platform_init_args
    add $8, %esp  /* Clean up stack (2 args * 4 bytes) */

    /* Now call main() with no arguments */
    call main

    /* If main returns, halt the CPU */
    cli              /* Disable interrupts */
halt_loop:
    hlt              /* Halt instruction */
    jmp halt_loop    /* Jump back in case of spurious wakeup */

.size _start, . - _start

/* Mark stack as non-executable */
.section .note.GNU-stack, "", @progbits
