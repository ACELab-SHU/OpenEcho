OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
	rom   (x!raiw) : ORIGIN = 0x00000000, LENGTH = 128k
	ram   (rw!xai) : ORIGIN = 0x00020000, LENGTH = 1M
}

/*
  #  https://sourceware.org/binutils/docs/ld/SECTIONS.html
  '.' = current memory location
 */
SECTIONS
{
	.text : ALIGN(64)
	{	
		PROVIDE(_text_start = .);
		KEEP(*(.start))
		*(.text*)
		PROVIDE(_text_end = .);
	} > rom

	combined_data : ALIGN(64)
	{	. = ALIGN(64);
        *(venus_global);
        *(venus_parameter);
		*(venus_dag_input);
		*(venus_dfedata);		
		. = ALIGN(64);
		end = .;
    } > ram
	
	.rodata : ALIGN(64)
	{
		PROVIDE(_rodata_start = .);
		*(.rodata .rodata.* .gnu.linkonce.r.*)   
		PROVIDE(_rodata_end = .); 
	} > ram

	.data : ALIGN(64)
	{
		PROVIDE(_data_start = .);
		*(.data .data.* .gnu.linkonce.d.*)   
		PROVIDE(_data_end = .); 
	} > ram

	.sdata : ALIGN(64)
	{
		PROVIDE(_sdata_start = .);
		*(.sdata .sdata.* .gnu.linkonce.s.*)
		PROVIDE(_sdata_end = .);
	} > ram

	.sbss : ALIGN(64)
	{
		PROVIDE(_sbss_start = .);
		*(.dynsbss)
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.scommon)
		PROVIDE(_sbss_end = .);
	} > ram

	.bss : ALIGN(64)
	{
		PROVIDE(_bss_start = .);
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		PROVIDE(_bss_end = .);
	} > ram

	. = ALIGN(64);
	. = SEGMENT_START("ldata-segment", .);
	. = ALIGN(64);

	PROVIDE(_memory_start = ORIGIN(ram));
	PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));

	PROVIDE(_heap_start = _bss_end);
	PROVIDE(_heap_size = _memory_end - _heap_start);


}
