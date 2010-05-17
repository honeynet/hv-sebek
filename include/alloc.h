#ifndef __ALLOC_H__
#define __ALLOC_H__


#include "e820.h"
#include "pmem_layout.h"

struct naive_allocator
{
	unsigned long *alloc_bitmap;
	unsigned long max_page;
};

extern void __init naive_memmap_init ( const struct e820_map *e820, struct pmem_layout *pml );
unsigned long alloc_host_pages ( unsigned long nr_pfns, unsigned long pfn_align );

#endif /* __ALLOC_H__ */
