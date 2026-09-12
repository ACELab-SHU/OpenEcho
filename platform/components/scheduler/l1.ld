OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
	/* 0x10000000 - 0x10020000 */
	/* cacheram   (wxa!ri) : ORIGIN = 0x10000000, LENGTH = 0x1ffff */
	cacheram   (wxa!ri) : ORIGIN = 0x10000000, LENGTH = 0xa0000
	uncacheram (wxa!ri) : ORIGIN = 0x100a0000, LENGTH = 0x60000
	/* ram (wxa!ri) : ORIGIN = 0x10000000, LENGTH = 0x1fffff */
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

/* 后面改一下这里 让dag在1010000之后就行 */
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



