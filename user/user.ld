OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS
{
    . = 0x10000;
    .text : {
         *(.text.entry)
         *(.text .text.*)
    }
    . = ALIGN(4K);
    .rodata : {
        *(.rodata .rodata.*)
    }
    . = ALIGN(4K);
    .data : {
        *(.data .data.*)
    }
    .bss : {
        *(.bss .bss.*)
    }
}