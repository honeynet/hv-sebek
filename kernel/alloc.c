/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#include "string.h"
#include "failure.h"
#include "serial.h"
#include "page.h"
#include "pmem_layout.h"
#include "e820.h"
#include "alloc.h"
#include "vmm.h"


enum {
	ALLOC_BITMAP_SHIFT = 6 /* 
					under -m32 option
					1 << ALLOC_BITMAP_SHIFT
				  = 1 << 5
				  = 32
				  = sizeof ( unsigned long ) * 8 
				  
					once ported to 64 bit
					1 << ALLOC_BITMAP_SHIFT
				  = 1 << 6
				  = 64
				  = sizeof( unsigned long ) * 8
				  */
};

static struct naive_allocator host_naive_allocator;

static inline unsigned long
get_alloc_bitmap_idx ( unsigned long pfn )
{
	return ( pfn >> ALLOC_BITMAP_SHIFT );
}

static inline unsigned long
get_alloc_bitmap_offset ( unsigned long pfn )
{
	return ( pfn & ( ( 1 << ALLOC_BITMAP_SHIFT ) - 1 ) );
}

static inline int 
allocated_in_map (const struct naive_allocator *nalloc, unsigned long pfn)
{
	const unsigned long *tbl   = nalloc->alloc_bitmap;
	const unsigned long idx    = get_alloc_bitmap_idx ( pfn );
	const unsigned long offset = get_alloc_bitmap_offset ( pfn );

	return ( tbl [ idx ] & ( 1UL << offset ) );
}

static void
map_alloc ( struct naive_allocator *nalloc, unsigned long first_page, unsigned long nr_pages )
{
	unsigned long *tbl = nalloc->alloc_bitmap;
	unsigned long start_off, end_off, curr_idx, end_idx;

//	outf ( "alloc: pfn=%x, size=%x\n", first_page, nr_pages ); /* [DEBUG] */

	curr_idx  = get_alloc_bitmap_idx ( first_page );
	start_off = get_alloc_bitmap_offset ( first_page );
	end_idx   = get_alloc_bitmap_idx ( first_page + nr_pages );
	end_off   = get_alloc_bitmap_offset ( first_page + nr_pages );

	if ( curr_idx == end_idx ) {
		/* set all n-th bits s.t. start_off <= n < end_off */
		tbl [ curr_idx ] |= ( ( ( 1UL << end_off ) - 1 ) &   /*  (1<<n)-1 sets all bits < n.  */
				      ( - ( 1UL << start_off ) ) );  /*  -(1<<n)  sets all bits >= n.  */
	} else  {
		tbl [ curr_idx ] |= - ( 1UL << start_off );
		for ( curr_idx += 1; curr_idx < end_idx; curr_idx++ ) {
			tbl [ curr_idx ] = ~0UL;
		}
		tbl [ curr_idx ] |= ( 1UL << end_off ) - 1;
	}
}

static void
map_free ( struct naive_allocator *nalloc, unsigned long first_page, unsigned long nr_pages )
{
	unsigned long *tbl = nalloc->alloc_bitmap;
	unsigned long start_off, end_off, curr_idx, end_idx;

//	outf ( "free: pfn=%x, size=%x\n", first_page, nr_pages );

	curr_idx  = get_alloc_bitmap_idx ( first_page );
	start_off = get_alloc_bitmap_offset ( first_page );
	end_idx   = get_alloc_bitmap_idx ( first_page + nr_pages );
	end_off   = get_alloc_bitmap_offset ( first_page + nr_pages );

	if ( curr_idx == end_idx ) {
		tbl [ curr_idx ] &= - ( 1UL << end_off ) | ( ( 1UL << start_off ) - 1 );
	} else {
		tbl [ curr_idx ] &= ( 1UL << start_off ) - 1;
		for ( curr_idx += 1; curr_idx < end_idx; curr_idx++ ) {
			tbl [ curr_idx ] = 0;
		}
		tbl [ curr_idx ] &= - ( 1UL << end_off );
	}
}

static void 
free_alloc_bitmap_region ( struct naive_allocator *nalloc, unsigned long _start, unsigned long _end )
{
	const unsigned long start = PAGE_UP ( _start );
	const unsigned long end   = PAGE_DOWN ( _end );

	if ( end <= start ) {
		return;
	}
	outf("Marking this memory range as Available: %x %x\n",_start,_end);
	map_free ( nalloc, start >> PAGE_SHIFT, ( end - start ) >> PAGE_SHIFT );
}

void 
free_host_alloc_bitmap_region ( unsigned long _start, unsigned long _end )
{
	free_alloc_bitmap_region(&host_naive_allocator, _start, _end);
}

//Mark VMM heap area as free in the host bitmap
// leave all regions outside VMM heap untouched (being marked as allocated)
// including host executable image, guest allocation bitmap, host allocation bitmap, AND guest memory
static void 
init_host_alloc_bitmap ( const struct e820_map *e820, const struct pmem_layout *pml )
{
	int i;
	for (i = 0; i < e820->nr_map; i++ )
	{
		const struct e820_entry *p = &e820->map[i];

		if (p->type != E820_RAM ) continue;

		unsigned long start = p->addr;
		unsigned long end = start + p->size;

		//skip regions which are totally outside VMM heap (pml->vmm_heap_start -> pml->vmm_pmem_end)
		if (end < pml->vmm_heap_start) continue;
		if (start > pml->vmm_pmem_end) continue;

		//trimming the regions which overlap with VMM heap, the result will be within VMM heap
		if (start < pml->vmm_heap_start) start = pml->vmm_heap_start;
		if (end > pml->vmm_pmem_end) end = pml->vmm_pmem_end;

		//mark region from start to end as free in host alloc bitmap
		free_host_alloc_bitmap_region (start, end);
	}
}

/*
//Anh - mark all pages after VM page_heap_end, which do not contain guest image copy
// as AVAILABLE in allocation bitmap nalloc
static void
init_alloc_bitmap ( const struct e820_map *e820, struct naive_allocator *nalloc, const struct pmem_layout *pml )
{
	int i;

	for ( i = 0; i < e820->nr_map; i++ ) {
		const struct e820_entry *p = &e820->map [ i ];

		if ( p->type != E820_RAM ) {
			continue;
		}

		unsigned long s = p->addr;
		unsigned long e = s + p->size;

		//Anh - leave all regions before vmm_heap_end mark as allocated
		//(guest physical memory area, allocation bitmap area, and vmm heap area)
		if (s < pml->vmm_heap_end) s = pml->vmm_heap_end;

		//Anh - mark the rest, except guest image copy region, as FREE
		const unsigned long guest_image_end = pml->guest_image_start + pml->guest_image_size;
		if ( ( s < guest_image_end ) && ( e > pml->guest_image_start ) ) {
			// free from s to the guest start
			__init_alloc_bitmap ( nalloc, s, pml->guest_image_start );
			// free memory after guest
			s = guest_image_end;
		}

		__init_alloc_bitmap ( nalloc, s, e );
	}
}
*/

void __init 
naive_memmap_init (const struct e820_map *e820, struct pmem_layout *pml)
{
	extern unsigned long _end; /* _end is the standard ELF symbol */

	//Host allocation bitmap (hostab): bitmap to manage MAVMM data
	//This starts right after VMM code
//	const unsigned long host_ab_start = PAGE_UP (PHYS(_end));

	//TODO: see why _end messed up
	const unsigned long host_ab_start = PAGE_UP (0x8100000);

	/* Allocate space for the allocation bitmap. Each bit in the bitmap specify whether a corresponding
	 * 4KB page is free or not. (0 = free, 1 = occupied)
	 * Add an extra longword of padding for possible overrun in map_alloc and map_free. ?? */
	// pml->max_page bits => pml->max_page / 8 bytes
	const unsigned long mapsize_bytes = pml->max_page / 8;
	const unsigned long mapsize_rounded = PAGE_UP (mapsize_bytes + sizeof (unsigned long));

	// VMM heap start after the allocation bitmaps
	pml->vmm_heap_start = host_ab_start + mapsize_rounded;

	//Debug
	outf("\n++++++ VMM physical memory map\n");
	outf("VMM start at: %x\n", pml->vmm_pmem_start);
	outf("_end of VMM code: %x\n", _end);
	outf("VMM memory bitmap start at: %x\n", host_ab_start);
	outf("VMM heap start at: %x\n", pml->vmm_heap_start);
	outf("VMM end at: %x\n", pml->vmm_pmem_end);

	if (pml->vmm_heap_start >= pml->vmm_pmem_end) fatal_failure ("No space left for VMM heap.\n");

	//Anh - Init host alloc bitmap
	struct naive_allocator *nalloc = &host_naive_allocator;
	{
		nalloc->alloc_bitmap = (unsigned long *) host_ab_start;
		nalloc->max_page = pml->max_page;

		//Mark all as allocated (not available) by default. Note: bitmap = 1 => NOT available
		memset ( nalloc->alloc_bitmap, ~0, mapsize_rounded );
	}

	outf("\nInitialize VMM memory allocation bitmap:\n");
	//Now mark VMM heap area as free (=0) in the host bitmap
	init_host_alloc_bitmap (e820, pml);
}

static int 
is_free_contiguous_region (const struct naive_allocator *nalloc, unsigned long pfn, unsigned long nr_pfns)
{
	unsigned long i;

	for (i = 0; i < nr_pfns; i++)
		if (allocated_in_map (nalloc, pfn + i))	return 0;

	return 1;
}


//Anh - find a free and continuous region of nr_pfns pages
// which start at a page number multiple of pfn_align
// mark these pages as allocated in allocation bitmap, then return the first PAGE NUMBER
//Note: halt if can not find any suitable region <--Todo: create another less crucial func
static unsigned long 
alloc_pages (unsigned long nr_pfns, unsigned long pfn_align, struct naive_allocator * nalloc)
{
	//struct naive_allocator *nalloc = &naive_allocator;
	unsigned long i;

	for (i = 0; i + nr_pfns < nalloc->max_page; i += pfn_align)
	{
		if (is_free_contiguous_region (nalloc, i, nr_pfns))
		{
			//outf("Free region: start %x, no of pages %x\n", i, nr_pfns);
			map_alloc (nalloc, i, nr_pfns);
			return i;
		}
	}

	fatal_failure ("alloc_pages failed\n");
	return 0;
}

// Anh: separate alloc_host and alloc_guest
// Move all host allocations (VMCB, IO and MSR intercept bitmap...) to host heap
// alloc e820 structure in guest memory, then copy host e820 and hide host memory

// Alloc pages inside host (VMM) memory
unsigned long 
alloc_host_pages ( unsigned long nr_pfns, unsigned long pfn_align )
{
	return alloc_pages ( nr_pfns, pfn_align, &host_naive_allocator);
}

