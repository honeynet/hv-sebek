/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "string.h"
#include "failure.h"
#include "serial.h"
#include "page.h"
#include "alloc.h"
#include "system.h"

unsigned long pg_table_alloc (void)
{
	const unsigned long pfn   = alloc_host_pages (1, 1);
	const unsigned long paddr = pfn << PAGE_SHIFT;

	memset ((char *) paddr, 0, PAGE_SIZE );

	return paddr;
}

unsigned long pml4_table_alloc (void)
{
	return pg_table_alloc();
}

static unsigned long get_index_pae2mb ( unsigned long vaddr, enum pg_table_level level )
{
	unsigned long shift = 0;
	const unsigned long MASK = ( ( 1 << 9 ) - 1 );

	switch ( level ) {
	case PGT_LEVEL_PML4: shift = 39; break;
	case PGT_LEVEL_PDP:  shift = 30; break;
	case PGT_LEVEL_PD:   shift = 21; break;
	default:             fatal_failure ( "get_index_pae2mb get_index_pae2mb wrong level\n" ); break;
	}

	return ( vaddr >> shift ) & MASK;
}


static unsigned long get_index_4kb ( unsigned long vaddr, enum pg_table_level level )
{
	unsigned long shift = 0;
	const unsigned long MASK = ( ( 1 << 10 ) - 1 );

	switch ( level ) {
	case PGT_LEVEL_PD:   shift = 22; break;
	case PGT_LEVEL_PT:   shift = 12; break;
	default:             fatal_failure ( "get_index_4kb wrong level\n" ); break;
	}

	return (vaddr >> shift ) & MASK;
}

static union pgt_entry_2mb * get_entry (unsigned long pg_table_base_vaddr, unsigned long vaddr, enum pg_table_level level)
{
	const unsigned long index = get_index_pae2mb ( vaddr, level );
	return ( union pgt_entry_2mb *) ( pg_table_base_vaddr + index * sizeof ( union pgt_entry_2mb ) );
}

static int entry2mb_is_present (const union pgt_entry_2mb *x)
{
	return x->term.flags & PTTEF_PRESENT;
}

static int entry4mb_is_present (const struct pd4M_entry *x)
{
	return x->flags & PTTEF_PRESENT;
}

//haind
static int entry4kb_is_present ( const union pgt_entry_4kb *x )
{
	return x->pte.flags & PTTEF_PRESENT;
}


void mmap_4mb ( unsigned long pg_table_base_vaddr, unsigned long vaddr, unsigned long paddr,
		int flags )
{
	//outf ( "__mmap: vaddr=%x, paddr=%x.\n", vaddr, paddr );

	int index = vaddr >> PAGE_SHIFT_4MB;
	struct pd4M_entry * entry = (struct pd4M_entry *) (pg_table_base_vaddr + index * sizeof(struct pd4M_entry));

	/* For page directory entry */
	unsigned int base = paddr >> PAGE_SHIFT_4MB;
	//TODO: handle base > 32 bits
	entry->baselow = base;
	entry->basehigh = 0;

	entry->rsvr = 0;

	if (flags == -1) entry->flags = PTTEF_PRESENT | PTTEF_RW | PTTEF_PAGE_SIZE | PTTEF_US;
	else entry->flags = flags;

//	outf ( "__mmap: table=%x, vaddr=%x, paddr=%x, flag=%x.\n", pg_table_base_vaddr, vaddr, paddr, entry->flags);
}


//haind: get a pointer to an entry of page directory or page table
static union pgt_entry_4kb * get_entry4kb (unsigned long pg_table_base_vaddr, unsigned long vaddr, enum pg_table_level level)
{
	const unsigned long index = get_index_4kb (vaddr, level);
	return (union pgt_entry_4kb *) (pg_table_base_vaddr + index * sizeof (union pgt_entry_4kb));
}

static struct pd4M_entry * get_pt_entry4mb (unsigned long pg_table_base_hpaddr, unsigned long linearaddr)
{
	const unsigned long index = linearaddr >> 22;
//	outf("pd4M entry index = %x\n",index);

	return (struct pd4M_entry *) (pg_table_base_hpaddr + index * sizeof (struct pd4M_entry));
}


/* [Note] paging with compatibility-mode (long mode with 2-Mbyte page tranlation) is only supported.  */
void __mmap_2mb ( unsigned long pg_table_base_vaddr, unsigned long vaddr, unsigned long paddr,
		enum pg_table_level level, int is_user )
{
	outf ( "__mmap: level=%x, vaddr=%x, paddr=%x.\n", level, vaddr, paddr );

	union pgt_entry_2mb *entry = get_entry (pg_table_base_vaddr, vaddr, level);

	if (level == PGT_LEVEL_PD)
	{
		/* For page directory entry */
		entry->term.base = paddr >> PAGE_SHIFT_2MB;
		entry->term.flags = PTTEF_PRESENT | PTTEF_RW | PTTEF_PAGE_SIZE;
		if ( is_user ) { entry->term.flags |= PTTEF_US; }

		//printf ( "__mmap: level=%x, vaddr=%x, paddr=%x.\n", level, vaddr, paddr );

		return;
	}

	//Anh - Note: non terminating tables point to 4KB pages, not 2MB pages
	/* For page-map level-4 entry and page-directory-pointer entry */

	if ( ! entry2mb_is_present ( entry ) )
	{
		const unsigned long paddr = pg_table_alloc();
		entry->non_term.base  = paddr >> PAGE_SHIFT;
		entry->non_term.flags = PTTEF_PRESENT | PTTEF_RW | PTTEF_US;
	}

	// pg_table_base
	const unsigned long next_table_base_vaddr = ( unsigned long ) (entry->non_term.base << PAGE_SHIFT);

	__mmap_2mb ( next_table_base_vaddr, vaddr, paddr, level - 1, is_user );
}

void mmap_pml4 ( unsigned long pml4_table_base_vaddr, unsigned long vaddr, unsigned long paddr, int is_user )
{
	__mmap_2mb( pml4_table_base_vaddr, vaddr, paddr, PGT_LEVEL_PML4, is_user );
}

/******************************************************/

static unsigned long
__linear2physical_legacy2mb ( unsigned long pg_table_base_vaddr, unsigned long vaddr, enum pg_table_level level )
{
	union pgt_entry_2mb *e = get_entry ( pg_table_base_vaddr, vaddr, level );

	if ( ! entry2mb_is_present ( e ) ) {
		fatal_failure ( "Page table entry is not present.\n" );
	}

	if ( level == PGT_LEVEL_PD ) {

		/* For page directory entry */

		if ( ! ( e->term.flags & PTTEF_PAGE_SIZE ) ) {
			fatal_failure ( "Not 2 Mbyte page size.\n" );
		}

		return ( ( e->term.base << PAGE_SHIFT_2MB ) + ( vaddr & ( ( 1 << PAGE_SHIFT_2MB ) - 1 ) ) );
	}

	const unsigned long next_table_base_vaddr = ( unsigned long )  (e->non_term.base << PAGE_SHIFT);
	return __linear2physical_legacy2mb ( next_table_base_vaddr, vaddr, level - 1 );
}

//Anh - return host physical addr from guest physical addr, and a pointer to nested pml4 table
// if pml4_table_base_vaddr = 0 => nested paging was disabled
unsigned long linear2physical_legacy2mb (unsigned long pml4_table_base_vaddr, unsigned long vaddr )
{
	if (pml4_table_base_vaddr == 0) return vaddr;
	else return __linear2physical_legacy2mb ( pml4_table_base_vaddr, vaddr, PGT_LEVEL_PML4 );
}

/******************************************************/
void print_4MB_pg_table ( unsigned long pg_table_base_vaddr)
{
	int i;

	for (i = 0; i < 1024; i++ )
	{
		struct pd4M_entry *e = (struct pd4M_entry *) (pg_table_base_vaddr + i * sizeof (struct pd4M_entry));

		if (!entry4mb_is_present(e)) continue;

		outf ("index=%x, vaddr=%x, paddr = %x", i, i << PAGE_SHIFT_4MB,
				((u64) e->baselow << PAGE_SHIFT_4MB) + ((u64) e->basehigh << 32));
		outf (", flags=%x\n", e->flags );
	}
}

static void __print_pml4_2MB_pg_table ( unsigned long pg_table_base_vaddr, enum pg_table_level level )
{
	int i;

	for ( i = 0; i < 512; i++ ) {
		union pgt_entry_2mb *e = ( union pgt_entry_2mb *) ( pg_table_base_vaddr + i * sizeof ( union pgt_entry_2mb ) );

		if ( ! entry2mb_is_present ( e ) ) {
			continue;
		}

		if ( level == PGT_LEVEL_PD ) {
			outf ( "level=%x, index=%x, voffset=%x, paddr = %x, flags=%x PD\n" ,
				 level, i, i << PAGE_SHIFT_2MB, e->term.base << PAGE_SHIFT_2MB, e->term.flags );
		} else {
			char shift;
			switch (level)
			{
			case PGT_LEVEL_PML4:
				shift = 39;
				break;
			case PGT_LEVEL_PDP:
				shift = 30;
				break;
			case PGT_LEVEL_PD:
				//nothing? nabil
				break;
			default: fatal_failure("__print_pml4_2MB_pg_table wrong paging level");
			}

			//Anh - Note: non terminating tables point to 4KB pages, not 2MB pages
			outf ( "level=%x, index=%x, vbase=%x, paddr %x, flags=%x\n" ,
				 level, i, i << shift, e->non_term.base << PAGE_SHIFT, e->non_term.flags );

			const unsigned long next_table_base_vaddr = ( unsigned long ) (e->non_term.base << PAGE_SHIFT);
			__print_pml4_2MB_pg_table( next_table_base_vaddr, level - 1 );
		}
	}
}

void print_pml4_2MB_pg_table ( unsigned long pml4_table_base_vaddr )
{
	outf ( "-----------------------------------------\n" );
	__print_pml4_2MB_pg_table ( pml4_table_base_vaddr, PGT_LEVEL_PML4 );
	outf ( "-----------------------------------------\n" );
}

//haind the following functions help convert a linear address
//to a physical address & in particular convert a guest physical
//address to a host physical address

static long long __linear2physical_legacy4kb
	(unsigned long pml2_table_base_physicaladdr, unsigned long linearaddr, enum pg_table_level level );

static unsigned long ___linear2physical_legacy4kb
	(union pgt_entry_4kb *e, unsigned long linearaddr, enum pg_table_level level)
{
	unsigned long base = e->pte.base << PAGE_SHIFT_4KB;
//	outf("base = %x\n", base);

	/* For page table entry */
	if ( level == PGT_LEVEL_PT ) {
		unsigned long offset = linearaddr & ((1 << PAGE_SHIFT_4KB) - 1);
//		outf("offset = %x\n", offset);

		return (base + offset);
	}

	return __linear2physical_legacy4kb (base, linearaddr, level - 1);
}

//haind, Anh: convert linear addr to  physical addr, 4kb in use
static long long __linear2physical_legacy4kb
	(unsigned long pml2_table_base_physicaladdr, unsigned long linearaddr, enum pg_table_level level )
{
	union pgt_entry_4kb *e = get_entry4kb ( pml2_table_base_physicaladdr, linearaddr, level);

	if (!entry4kb_is_present(e)) {
		//TODO: check why page not present in some case (after enter username with tty)
		// fatal_failure ("Page table entry is not present.\n");
		return -1;
	}

	return ___linear2physical_legacy4kb(e, linearaddr, level);
}

//haind: convert linear addr to  physical addr, 4kb in use
long long linear2physical_legacy4kb (unsigned long pml2_table_base_physicaladdr, unsigned long linearaddr )
{
	return __linear2physical_legacy4kb ( pml2_table_base_physicaladdr, linearaddr, PGT_LEVEL_PD );
}

//haind, Anh: convert linear addr to  physical addr, 4mb in use
static long long __linear2physical_legacy4mb ( unsigned long pml2_table_base_physicaladdr, unsigned long linearaddr)
{
	struct pd4M_entry *e = get_pt_entry4mb (pml2_table_base_physicaladdr, linearaddr);

//	outf("entry ptr = %x\n", e);

//	u32 ue = *((u32 *) e);
//	outf("entry = %x\n", ue);

	if (e->flags & PTTEF_PAGE_SIZE) {
		// if PS flag is set in PDE => this page is 4MB
		// physical addr = base + offset
		if (!entry4mb_is_present (e))	{
			//TODO: check why page not present in some case (after enter username with tty)
//			fatal_failure ("Page directory entry is not present.\n");
			return -1;
		}

//		outf("entry's flag: %x\n", e->flags);
//		outf("basehigh = %x\n", e->basehigh);
//		outf("baselow = %x\n", e->baselow);

		//TODO: handle basehigh and base > 4GB
		unsigned long base = e->baselow << 22;
//		outf("base = %x\n", base);

		unsigned long offset = linearaddr & ((1 << PAGE_SHIFT_4MB) - 1);
//		outf("offset = %x\n", offset);
		return base + offset;
	}
	else { //Anh - PS not set does not mean an error, AMD man vol2, p48
		// if PS flag is not set in PDE => this page is 4KB
//		outf("This is a 4KB page\n");
		return ___linear2physical_legacy4kb((union pgt_entry_4kb *) e, linearaddr, PGT_LEVEL_PD);
	}
}

//haind: convert linear addr to  physical addr, 4mb in use
long long linear2physical_legacy4mb (unsigned long pml2_table_base_physicaladdr, unsigned long linearaddr)
{
//	breakpoint("start calling convert");
	return __linear2physical_legacy4mb ( pml2_table_base_physicaladdr, linearaddr);
}

u64 linear_2_physical(u64 cr0, u64 cr3, u64 cr4, u64 guest_linear)
{
	u8 pe = (cr0 & X86_CR0_PE) > 0;

	//get segment selector and offset in case segmentation is in use
	u64 guest_physical;

	if (pe == 0)//real mode
		guest_physical = guest_linear;
	else //protected mode
	{
		u8 pg = (cr0 & X86_CR0_PG) > 0;
		u8 pse = (cr4 & X86_CR4_PSE) > 0;

		if (cr4 & X86_CR4_PAE) {
			fatal_failure("Converting address in 2mb PAE paging mode not supported yet!");
		}

		if (pg == 0) // no guest paging => GP = GLinear
			guest_physical = guest_linear;
		else
		{
			//the following instructions work on a hypothesis that long mode
			//isn't in use and page address extension is disabled
			//first determining whether 4 kb pages or 4 mb pages are being used
			//reading page 123 of AMD vol 2 for more details
			//extracting the bit PSE of cr4

			// cr3 contains guest physical
			// since GP = HP => cr3 contains HP of the guest paging structure
			if (pse==0)//4kb
				guest_physical = linear2physical_legacy4kb(cr3, guest_linear);
			else //4mb
				guest_physical = linear2physical_legacy4mb(cr3, guest_linear);
		}

//		outf("==> guest physical: %x\n", guest_physical);
	}

	return guest_physical;
}
