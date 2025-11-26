/* linker.ld - Linker script for bare-metal x86 kernel */

ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

/* Define program headers */
PHDRS
{
    boot PT_LOAD FLAGS(5);   /* Read + Execute for boot code */
    text PT_LOAD FLAGS(5);   /* Read + Execute */
    rodata PT_LOAD FLAGS(4); /* Read only */
    data PT_LOAD FLAGS(6);   /* Read + Write */
}

SECTIONS
{
    /* Kernel starts at 1MB (standard for x86) */
    . = 1M;

    /* Multiboot header MUST be in first 8KB and in a LOAD segment */
    .multiboot ALIGN(4) : {
        *(.multiboot)
    } :boot

    /* PVH note for QEMU 10.x - in same segment as multiboot */
    .note.Xen ALIGN(4) : {
        *(.note.Xen)
    } :boot

    /* Code section - align to page boundary, new segment */
    . = ALIGN(4K);
    .text : {
        *(.text*)
    } :text

    /* Read-only data - align to page boundary */
    . = ALIGN(4K);
    .rodata : {
        *(.rodata*)
    } :rodata

    /* Fixture data - embedded binary files */
    . = ALIGN(4K);
    .rodata.fixtures : {
        __fixtures_section_start = .;
        *(.rodata.fixtures)
        __fixtures_section_end = .;
    } :rodata

    /* Initialized data - align to page boundary */
    . = ALIGN(4K);
    .data : {
        *(.data*)
    } :data

    /* Uninitialized data (BSS) */
    .bss : {
        *(COMMON)
        *(.bss*)
    } :data

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
        *(.note.gnu.property)
    }
}
