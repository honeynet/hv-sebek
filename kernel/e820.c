/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "types.h"
#include "failure.h"
#include "serial.h"
#include "page.h"
#include "multiboot.h"
#include "e820.h"
#include "string.h"


static void __init
add_memory_region ( struct e820_map *e820, u64 start, u64 size, enum e820_type type )
{
	if ( e820->nr_map >= E820_MAX_ENTRIES ) {
		fatal_failure ( "Too many entries in the memory map.\n" );
		return;
	}

	struct e820_entry *p = &e820->map [ e820->nr_map ];
	p->addr = start;
	p->size = size;
	p->type = type;

	e820->nr_map++;
}

void __init
e820_print_map ( const struct e820_map *e820 )
{
	int i;

	outf ( "BIOS-provided physical RAM map:\n" );

	for ( i = 0; i < e820->nr_map; i++ ) {
		const struct e820_entry *p = &e820->map [ i ];

		outf(" %x -", (unsigned long long) p->addr);
		outf(" %x ", (unsigned long long) ( p->addr + p->size ));
		switch ( p->type ) {
		case E820_RAM:			outf ( "(usable)\n" ); break;
		case E820_RESERVED: 	outf ( "(reserved)\n" ); break;
		case E820_ACPI:			outf ( "(ACPI data)\n" ); break;
		case E820_NVS:  		outf ( "(ACPI NVS)\n" ); break;
		default:				outf ( "type %x\n", p->type ); break;
		}
	}
}

void __init setup_memory_region ( struct e820_map *e820, const struct multiboot_info *mbi )
{
	//first check that mbi->mmap_* fields are valid
	//check http://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
	if (!(mbi->flags & MBI_MEMMAP)) fatal_failure ( "Bootloader provided no memory information.\n" );

	e820->nr_map = 0;

	unsigned long p = 0;
	// go through memory_map entries in mbi, from mmap_addr to mmap_addr + mmap_length
	while ( p < mbi->mmap_length )
	{
		const struct memory_map *mmap = (struct memory_map *) (mbi->mmap_addr + p);

		const u64 start = ( (u64)(mmap->base_addr_high) << 32) | (u64) mmap->base_addr_low;
		const u64 size = ( (u64)(mmap->length_high) << 32) | (u64) mmap->length_low;

		//Debug
		//outf("start: %x - size: %x (%x, %x) - type: %x\n", start, size, mmap->length_low, mmap->length_high, mmap->type);

		//for each memory_map entry, add one corresponding entry to e820
		add_memory_region ( e820, start, size, mmap->type );

		//go to next memory_map entry
		p += mmap->size + sizeof (mmap->size);
	}

	// DEBUG
	e820_print_map(e820);
}

// get the number of memory pages usable as RAM (has E820_RAM type)
unsigned long __init get_nr_pages ( const struct e820_map *e820 )
{
	unsigned long n = 0;
	int i;

	for ( i = 0; i < e820->nr_map; i++ ) {
		const struct e820_entry *p = &e820->map [ i ];

		if (p->type != E820_RAM) continue;

		n += p->size >> PAGE_SHIFT;
	}

	return n;
}

// get the page number of the LAST memory page which is usable as RAM (has E820_RAM type)
unsigned long __init get_max_pfn ( const struct e820_map *e820 )
{
	unsigned long n = 0;
	int i;

	for ( i = 0; i < e820->nr_map; i++ ) {
		const struct e820_entry *p = &e820->map [i];

		if ( p->type != E820_RAM ) continue;

		const unsigned long start = PFN_UP ( p->addr );
		const unsigned long end = PFN_DOWN ( p->addr + p->size );

		if (( start < end ) && ( end > n )) n = end;
	}

	return n;
}


//Anh - hide a memory region by marking it as reserved
void hide_memory ( struct multiboot_info * mbi, unsigned long hidestart, unsigned long length )
{
	unsigned long map_base = (unsigned long ) mbi->mmap_addr;
	unsigned long hideend = hidestart + length - 1;
	u64 replace_start, replace_size = 0;

	outf("Hiding the host - start: %x, length: %x\n", hidestart, length);

	u32 offset = 0;
	while (offset < mbi->mmap_length)
	{
		struct memory_map * mmap = (struct memory_map *) (map_base + offset);
		offset += mmap->size + sizeof(mmap->size);

		if ( mmap->type != E820_RAM )
		{
			// if has replace request and found an E820_RESERVED region to replace
			if ((replace_size != 0) && (mmap->type == E820_RESERVED))
			{
				outf("Replacing mmap entry, original start: %x, original size: %x, new start: %x, new size: %x\n",
						mmap->base_addr_low, mmap->length_low, replace_start, replace_size);
				mmap->type = E820_RAM;

				mmap->base_addr_low = replace_start;
				mmap->base_addr_high = replace_start >> 32;

				mmap->length_low = replace_size;
				mmap->length_high = replace_size >> 32;

				replace_size = 0; //clear replace request
			}

			continue;
		}

		u64 start = ( (u64) mmap->base_addr_high << 32 ) | (u64) mmap->base_addr_low;
		u64 size = ( (u64) mmap->length_high << 32 ) | (u64) mmap->length_low;
		u64 end = start + size;

		//Case 1: e820 regions cover hiding region => has to split
		if (start < hidestart && end > hideend)
		{
			outf("This original region was splited - start: %x, size: %x\n", start, size);

			// 1st split
			size = hidestart - start;
			mmap->length_low = size;
			mmap->length_high = size >> 32;

			outf("First split - start: %x, size: %x\n", start, size);

			// 2nd split
			start = hideend + 1;
			size = end - start;

			// Anh - !!!!!! CANT DO THIS BECAUSE GRUB STORES OTHER DATA RIGHT AFTER MEMORY MAP
			// The best solution is to copy mbi to host memory, then create another one
			// for guest, at the same addr that was passed by GRUB

			/*
			//creating a new entry
			struct memory_map newentry;
			newentry.size = sizeof(newentry) - sizeof(newentry.size);
			newentry.base_addr_low = start;
			newentry.base_addr_high = start >> 32;
			newentry.length_low = size;
			newentry.length_high = size >> 32;

			//move existing stuff down - has to do 2 memcpy otherwise stuffs might be overwritten
			//Note: this might overwrite other stuffs which are allocted right after the memory map???
			memcpy((char *) map_base + mbi->mmap_length, (char *) map_base + offset, mbi->mmap_length - offset);
			memcpy((char *) map_base + offset + sizeof(newentry), (char *) map_base + mbi->mmap_length, mbi->mmap_length - offset);

			//copy new entry
			memcpy((char *) map_base + offset, &newentry, sizeof(newentry));
			// fixing map size
			mbi->mmap_length += sizeof(newentry);

			//skip newly created entry
			offset += sizeof(newentry);
			*/

			// Anh - create an insert request, which will replace the map for next reserved region
			// by the map for this 2nd splitted region
			replace_size = size;
			replace_start = start;

			outf("Second split - start: %x, size: %x\n", start, size);
			continue;
		}

		//Case 2: hiding region covers or overlaps e820 region => don't have to split
		char b_interleave = 0;
		u64 newstart, newend;

		if ( (start >= hidestart) && (start <= hideend) )
		{
			newstart = hideend + 1; //Anh - Move newstart to after hiding region
			b_interleave = 1;
		}
		if ( (end >= hidestart) && (end <= hideend) )
		{
			newend = hidestart - 1; // Move newend to before hiding region
			b_interleave = 1;
		}

		if (b_interleave == 0) continue;

		if (end > start)	//Anh - if orginial e820 region interleves hiding region
		{
			outf("This region was adjusted - original start: %x, original size: %x, new start: %x, new size: %x\n",
					start, size, newstart, newend - newstart);

			mmap->base_addr_low = newstart;
			mmap->base_addr_high = newstart >> 32;

			size = newend - newstart;
			mmap->length_low = size;
			mmap->length_high = size >> 32;
		}
		else  //Anh - otherwise orginial e820 region is totally inside hiding region
		{
			mmap->type = E820_RESERVED;
			outf("This region was hidden - start: %x, size: %x\n", start, size);
		}
	}
}
