#ifndef __PAGE_H__
#define __PAGE_H__

#define PAGE_SHIFT 12
#define PAGE_SIZE  ( 1 << PAGE_SHIFT )
#define PAGE_MASK  ( ~ ( PAGE_SIZE - 1 ) )

#define PAGE_SHIFT_2MB 21
#define PAGE_SIZE_2MB	(1 << PAGE_SHIFT_2MB)
#define PAGE_MASK_2MB	(~(PAGE_SIZE_2MB - 1))

//haind for 4kb page
#define PAGE_SHIFT_4KB 12
#define PAGE_SIZE_4KB (1 << PAGE_SHIFT_4KB)

#define PAGE_SHIFT_4MB 22
#define PAGE_SIZE_4MB  ( 1 << PAGE_SHIFT_4MB )
#define PAGE_MASK_4MB  ( ~ ( PAGE_SIZE_4MB - 1 ) )

#define PFN_UP(x)	(((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)

#define PFN_UP_2MB(x)	(((x) + PAGE_SIZE_2MB - 1) >> PAGE_SHIFT_2MB)
#define PFN_DOWN_2MB(x)	((x) >> PAGE_SHIFT_2MB)

#define PAGE_UP_2MB(p)		(((p) + (PAGE_SIZE_2MB - 1)) & PAGE_MASK_2MB)
#define PAGE_DOWN_2MB(p)	((p) & PAGE_MASK_2MB)

#define PAGE_UP_4MB(p)    ( ( (p) + ( PAGE_SIZE_4MB - 1 ) ) & PAGE_MASK_4MB )	// round up to closest next 4MB page boundary
#define PAGE_DOWN_4MB(p)  ( (p) & PAGE_MASK_4MB )		// round down to closest previous 4MB page boundary

#define PAGE_UP(p)    ( ( (p) + ( PAGE_SIZE - 1 ) ) & PAGE_MASK )	// round up to closest next 4KB page boundary
#define PAGE_DOWN(p)  ( (p) & PAGE_MASK )		// round down to closest previous 4KB page boundary

/* Page-Translation-Table Entry Fields
   [REF] vol.2, p. 168- */
#define _PTTEF_PRESENT   0
#define _PTTEF_RW        1 /* Read/Write */
#define _PTTEF_US        2 /* User/Supervisor */
#define _PTTEF_ACCESSED  5
#define _PTTEF_DIRTY     6
#define _PTTEF_PAGE_SIZE 7
#define _PTTEF_GLOBAL	 8
#define PTTEF_PRESENT    (1 << _PTTEF_PRESENT)
#define PTTEF_RW         (1 << _PTTEF_RW)
#define PTTEF_US         (1 << _PTTEF_US)
#define PTTEF_ACCESSED   (1 << _PTTEF_ACCESSED)
#define PTTEF_DIRTY      (1 << _PTTEF_DIRTY)
#define PTTEF_PAGE_SIZE  (1 << _PTTEF_PAGE_SIZE)
#define PTTEF_GLOBAL     (1 << _PTTEF_GLOBAL)


#ifdef __ASSEMBLY__

#define VMM_OFFSET	0xFFFF830000000000
#define PHYS(va)	((va) - VMM_OFFSET)

#else /* ! __ASSEMBLY__ */

#define VMM_OFFSET	0xFFFF830000000000UL
#define PHYS(va)	((unsigned long)(va) - VMM_OFFSET)
#define VIRT(pa)	((void *)((unsigned long)(pa) + VMM_OFFSET))

#endif /* __ASSEMBLY__ */


#ifndef __ASSEMBLY__


enum pg_table_level {
	PGT_LEVEL_PML4 = 4,
	PGT_LEVEL_PDP  = 3,
	PGT_LEVEL_PD   = 2,
	PGT_LEVEL_PT   = 1 //haind for page table
};

/*Anh - For 4MB page translation, PAE disabled, vol2 p124  */
struct pd4M_entry {
	u16 flags:  	13; /* Bit 0-12 */
	u16 basehigh:  	8; 	/* Bit 13-20 of the entry => bit 32-39 of base */
	u8  rsvr:  		1; 	/* Bit 21 */
	u16 baselow:  	10;	/* Bit 22-31 of the entry => bit 22-31 of base */
} __attribute__ ((packed));

/*haind - For 4KB page translation, PAE disabled*/
union pgt_entry_4kb
{

	struct pde {
		u32 flags: 12; /* Bit 0-11  */
		u32 base:  20; /* Bit 12-31 */

	} __attribute__ ((packed)) pde;

	struct pte {
			u32 flags: 12; /* Bit 0-11  */
			u32 base:  20; /* Bit 12-31 */

	} __attribute__ ((packed)) pte;
};


/* [REF] AMD64 manual Vol. 2, pp. 166-167 */

/* For 2-Mbyte page translation (long-mode) */
union pgt_entry_2mb
{
	/* 2-Mbyte PML4E and PDPE */
	struct non_term {
		u16 flags: 12; /* Bit 0-11  */
		u64 base:  40; /* Bit 12-51 */
		u16 avail: 11; /* Bit 52-62 */
		u16 nx:    1;  /* Bit 63    */
	} __attribute__ ((packed)) non_term;

	/* 2-Mbyte PDE */
	struct term {
		u32 flags: 21; /* Bit 0-20  */
		u32 base:  31; /* Bit 21-51 */
		u16 avail: 11; /* Bit 52-62 */
		u16 nx:    1;  /* Bit 63    */
	} __attribute__ ((packed)) term;
};

unsigned long pml4_table_alloc ( void );
unsigned long pg_table_alloc ( void );

extern void mmap_pml4 ( unsigned long pml4_table_base_vaddr, unsigned long vaddr, unsigned long paddr, int is_user );
extern void mmap_4mb ( unsigned long pg_table_base_vaddr, unsigned long vaddr, unsigned long paddr,	int is_user );

//TODO: use u64, u32.. instead of long long / prepare for 64 bit guest
extern u64 linear_2_physical(u64 cr0, u64 cr3, u64 cr4, u64 guest_linear);
extern long long linear2physical_legacy4kb ( unsigned long pg_table_base_vaddr, unsigned long vaddr);
extern long long linear2physical_legacy4mb ( unsigned long pg_table_base_vaddr, unsigned long vaddr);
extern unsigned long linear2physical_2mb ( unsigned long pml4_table_base_vaddr, unsigned long vaddr );

extern void print_pml4_2MB_pg_table ( unsigned long pml4_table_base_vaddr );
extern void print_4MB_pg_table ( unsigned long pg_table_base_vaddr);

#endif /* ! __ASSEMBLY__ */

#endif /* __PAGE_H__ */
