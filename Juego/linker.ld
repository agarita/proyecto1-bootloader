OUTPUT_FORMAT("elf32-i386");
ENTRY(boot)
SECTIONS {
	. = 0x7c00;
	.text :
	{
        *(.boot)
		*(.text)
	}

    .rodata :
    {
    	*(.rodata)
    }

    .data :
    {
    	*(.data)
    }

    .bss :
    {
    	*(.bss)
    }
}
