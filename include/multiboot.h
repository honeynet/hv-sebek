#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__


#include "types.h"

/* The magic number for the Multiboot header.  */
#define MULTIBOOT_HEADER_MAGIC		0x1BADB002

/* The flags for the Multiboot header.  */
//See http://www.gnu.org/software/grub/manual/multiboot/html_node/Header-magic-fields.html
// # define MULTIBOOT_HEADER_FLAGS		0x00010003
#define MULTIBOOT_HEADER_FLAGS		0x00000003

/* The magic number passed by a Multiboot-compliant boot loader.  */
#define MULTIBOOT_BOOTLOADER_MAGIC	0x2BADB002

#define MBI_CMDLINE    (1 << 2)
#define MBI_MODULES    (1 << 3)
#define MBI_MEMMAP     (1 << 6)

/* Do not include here in boot.S.  */
#ifndef __ASSEMBLY__

/* The symbol table for a.out.  */
struct aout_symbol_table
{
	u32 tabsize;
	u32 strsize;
	u32 addr;
	u32 reserved;
};

/* The section header table for ELF.  */
struct elf_section_header_table
{
	u32 num;
	u32 size;
	u32 addr;
	u32 shndx;
};

/* The Multiboot information.  */
// check http://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
struct multiboot_info
{
	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	u32 cmdline;
	u32 mods_count;
	u32 mods_addr;
	union
	{
		struct aout_symbol_table aout_sym;
		struct elf_section_header_table elf_sec;
	} u;
	u32 mmap_length;
	u32 mmap_addr;
};

/* The module structure.  */
struct module_info
{
	u32 mod_start;
	u32 mod_end;
	u32 string;
	u32 reserved;
};

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size.  */
struct memory_map
{
	u32 size;
	u32 base_addr_low;
	u32 base_addr_high;
	u32 length_low;
	u32 length_high;
	u32 type;
};

#endif /* ! __ASSEMBLY__ */

#endif /* __MULTIBOOT_H__ */
