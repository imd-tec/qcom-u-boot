/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * U-Boot riscv64 EFI application linker script
 *
 * Modified from lib/efi_loader/elf_efi.ldsi and
 * arch/arm/lib/elf_aarch64_efi.lds
 */

OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
	/*
	 * The .text section must come first at VMA 0 so that the PE/COFF
	 * header in .text.head is at the start of the binary.  The CRT0
	 * defines image_base at the very beginning of .text.head.
	 */
	.text 0x0 : {
		_text = .;
		*(.text.head)
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
		*(.srodata)
		*(.rodata*)
		. = ALIGN(16);
		*(.dynamic);
		. = ALIGN(512);
	}
	. = ALIGN(4096);
	_etext = .;
	_text_size = . - _text;

	/*
	 * .hash and .gnu.hash are needed in the ELF for dynamic linking
	 * during the link step, but are not extracted into the PE binary.
	 */
	.hash : { *(.hash) }
	.gnu.hash : { *(.gnu.hash) }
	.eh_frame : { *(.eh_frame) }

	. = ALIGN(65536);
	.reloc : {
		KEEP (*(.reloc))
	}

	/*
	 * Everything from _data to _edata makes up the PE .data section.
	 * The CRT0 PE header uses _edata for both VirtualSize and
	 * SizeOfRawData, so BSS must be included (as zero-filled file
	 * data) rather than in a separate section.
	 */
	. = ALIGN(4096);
	_data = .;

	/*
	 * Dynamic relocations in .rela.dyn so the linker sets DT_RELA
	 * to point here.  All relocation input sections are gathered
	 * into this single output section.
	 */
	.rela.dyn : {
		*(.rela.dyn)
		*(.rela.text*)
		*(.rela.data*)
		*(.rela.got)
		*(.rela.stab)
		*(.rela.init_array*)
		*(.rela.fini_array*)
		*(.rela.ctors*)
		*(.rela.dtors*)
		*(.rela__u_boot_list*)
	}

	/* Dynamic symbol table and strings */
	.dynsym   : { *(.dynsym) }
	.dynstr   : { *(.dynstr) }

	/* Embedded device tree (must be separate for embeddtb command) */
	. = ALIGN(16);
	.embedded_dtb : {
		*(.embedded_dtb)
	}

	/*
	 * Regular data and BSS live together in one output section.
	 * Mixing PROGBITS (.data) and NOBITS (.bss) input sections in
	 * the same output section forces the linker to emit file content
	 * for the entire section, so BSS gets zero-filled in the file.
	 * This avoids a VMA gap that would bloat the PE binary.
	 */
	.data : {
		*(.sdata)
		*(.data)
		*(.data1)
		*(.data.*)
		*(.got.plt)
		*(.got)

		/* U-Boot lists */
		. = ALIGN(8);
		KEEP(*(SORT(__u_boot_list*)));
		. = ALIGN(8);

		/* BSS merged into data to get file content */
		. = ALIGN(16);
		_bss = .;
		*(.sbss)
		*(.scommon)
		*(.dynbss)
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(512);
		_bss_end = .;
		_edata = .;
		_end = .;
	}
	_data_size = _edata - _data;

	/*
	 * PLT relocations are placed after _edata so they do not fall
	 * within the DT_RELA range.  _relocate() only handles
	 * R_RISCV_RELATIVE and would panic on R_RISCV_JUMP_SLOT entries.
	 */
	.rela.plt : { *(.rela.plt) }

	. = ALIGN(4096);
	.note.gnu.build-id : { *(.note.gnu.build-id) }
	/DISCARD/ : {
		*(.rel.reloc)
		*(.eh_frame)
		*(.note.GNU-stack)
	}
	.comment 0 : { *(.comment) }
}
