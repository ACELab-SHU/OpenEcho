OUTPUT_ARCH( "riscv" )

ENTRY( _start )

/*
 * Venus1.0 scheduler memory contract.
 *
 * The V1 full-SoC firmware shipped with the authoritative RTL keeps the
 * cacheable L1 image below 0x10100000 and places DAG/static payloads in the
 * uncached 0x10100000 aperture.  This file is selected by the V1 backend
 * manifest and copied only into the run-local Scheduler build tree.
 */
MEMORY
{
	cacheram   (wxa!ri) : ORIGIN = 0x10000000, LENGTH = 0xfffff
	uncacheram (wxa!ri) : ORIGIN = 0x10100000, LENGTH = 0x7ffff
}

SECTIONS
{
	.text : {
		PROVIDE(_text_start = .);
		. = 0x00000000;
		*(.startup)
		KEEP (*(.startup))
		. = 0x00000020;
		*(.startirq)
		KEEP (*(.startirq))
		*(.text .text.*)
		PROVIDE(_text_end = .);
	} >cacheram

	.rodata : {
		PROVIDE(_rodata_start = .);
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
		PROVIDE(_rodata_end = .);
	} >cacheram

	.data : {
		PROVIDE(_data_start = .);
		*(.sdata .sdata.*)
		*(.data .data.*)
		*(.bss.stack)
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		*(COMMON)
		PROVIDE(_data_end = .);
	} >cacheram

	.rfdata : {
		PROVIDE(_rfdata_start = .);
		*(.rfdata);
		PROVIDE(_rfdata_end = .);
	} > cacheram

	.bss :{
		PROVIDE(_bss_start = .);
		PROVIDE(_bss_end = .);
	} >cacheram

	.rfdata_data : {
		PROVIDE(_rfdata_data_start = .);
		*(.rfdata_data);
		PROVIDE(_rfdata_data_end = .);
	} > uncacheram

	.padding :{
		*(.padding)
	} >uncacheram

	.dag : {
		*(.dag)
	} > uncacheram

	_heap_start = ALIGN(64);
	PROVIDE(_memory_start = ORIGIN(uncacheram));
	PROVIDE(_memory_end = ORIGIN(uncacheram) + LENGTH(uncacheram));
	PROVIDE(_heap_end = _memory_end);
	_cacheram_end_addr = ORIGIN(cacheram) + LENGTH(cacheram);
}
