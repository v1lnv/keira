ENTRY(_start)
SECTIONS {
    . = 0x40000000; /* Load at 1GiB: PDPT[1] is unmapped, VMM can create clean 4KB mapping */
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
