OUTPUT_ARCH("riscv")
ENTRY(_entry)

SECTIONS
{
    /*. = 0x80000000;*/
    . = 0x80200000;
    .text : {
        *(.text .text.*)
         . = ALIGN(4k); 
        PROVIDE(kernel_text_end = .);
    }
    PROVIDE(kernel_end = .);
}