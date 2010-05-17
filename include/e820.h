#ifndef __E820_H__
#define __E820_H__


#include "types.h"
#include "multiboot.h"


enum {
	E820_MAX_ENTRIES = 128
};

enum e820_type {
	E820_RAM 	= 1,
	E820_RESERVED	= 2,
	E820_ACPI	= 3, /* usable as RAM once ACPI tables have been read */
	E820_NVS	= 4
};

struct e820_entry {
	u64 addr;		/* start of memory segment */
	u64 size;		/* size of memory segment */
	enum e820_type type;	/* type of memory segment */
} __attribute__((packed));

struct e820_map
{
	int nr_map;		// number of e820_entry entries
	struct e820_entry map [ E820_MAX_ENTRIES ];
};

extern void hide_memory ( struct multiboot_info * mbi, unsigned long hidestart, unsigned long length );
extern void __init setup_memory_region ( struct e820_map *e820, const struct multiboot_info *mbi );
extern unsigned long __init get_nr_pages ( const struct e820_map *e820 );
extern unsigned long __init get_max_pfn ( const struct e820_map *e820 );
extern void __init e820_print_map ( const struct e820_map *e820 );

#endif /* __E820_H__ */
