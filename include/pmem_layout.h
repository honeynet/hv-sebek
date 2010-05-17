#ifndef __PMEM_LAYOUT_H__
#define __PMEM_LAYOUT_H__

#include "e820.h"

struct pmem_layout {
	unsigned long max_page;
	unsigned long total_pages;

	unsigned long vmm_pmem_start, vmm_heap_start, vmm_pmem_end;

	struct e820_map e820;
};

#endif /* __PMEM_LAYOUT_H__ */
