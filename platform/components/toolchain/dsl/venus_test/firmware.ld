OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
	rom   (x!raiw) : ORIGIN = 0x00000000, LENGTH = 32k
	ram   (rw!xai) : ORIGIN = 0x00020000, LENGTH = 16k
}

SECTIONS
{
	.text : ALIGN(64)
	{	
		PROVIDE(_text_start = .);
		KEEP(*(.start))
		*(.text*)
		PROVIDE(_text_end = .);
	} > rom

combined_data :
{
    . = ALIGN(64); 
    PROVIDE(_rodata_start = .);
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    PROVIDE(_rodata_end = .);

    PROVIDE(_data_start = .);
    *(.data .data.* .gnu.linkonce.d.*)
    PROVIDE(_data_end = .);

	PROVIDE(_sdata_start = .);
	*(.sdata .sdata.* .gnu.linkonce.s.*)
	PROVIDE(_sdata_end = .);
	
    PROVIDE(_sbss_start = .);
    *(.dynsbss .sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
    PROVIDE(_sbss_end = .);

    PROVIDE(_bss_start = .);
    *(.dynbss .bss .bss.* .gnu.linkonce.b.* COMMON)
    PROVIDE(_bss_end = .);



    . = ALIGN(64);
} > ram

	. = ALIGN(64);
	. = SEGMENT_START("ldata-segment", .);
	. = ALIGN(64);

	PROVIDE(_memory_start = ORIGIN(ram));
	PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));

	PROVIDE(_heap_start = _bss_end);
	PROVIDE(_heap_size = _memory_end - _heap_start);


}
