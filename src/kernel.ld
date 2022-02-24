OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS
{
    /*. = 0x80000000;*/
    . = 0x80200000;
    .text : {
        *(.text.entry);
        . = ALIGN(4k);
        PROVIDE(trampoline_start = .);
        *(.text.trampoline);
        . = ALIGN(4k);
        *(.text .text.*)
         . = ALIGN(4k); 
        PROVIDE(kernel_text_end = .);
    }

    .rodata : {
        *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
        . = ALIGN(16);
        *(.rodata .rodata.*)
        . = ALIGN(4k);
        PROVIDE(kernel_rodata_end = .);
    }

      .data : {
        *(.sdata .sdata.*) /* do not need to distinguish this from .data */
        . = ALIGN(16);
        *(.data .data.*)
        . = ALIGN(16);
        PROVIDE(kernel_data_end = .);
    }

    .bss : {
        *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
        . = ALIGN(16);
        *(.bss .bss.*)
        . = ALIGN(16);
    }
    PROVIDE(kernel_end = .);
}