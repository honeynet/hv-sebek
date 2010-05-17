#ifndef __VMM_H__
#define __VMM_H__

#define STACK_SIZE	(1 << 16) /* 64 KB */
#define	DEFAULT_VMM_PMEM_SIZE (1 << 24) /* 16 MB */
#define	DEFAULT_VMM_PMEM_START  (1 << 27) /* 128 MB */

#define VMM_CS32	8 	// entry 1 of gdt //Anh - 00001000 => index = 1
#define VMM_DS32	16 	// entry 2 of gdt //Anh - 00010000 => index = 10

#define GDT_ENTRIES	12 /* ??? */


#endif /* __VMM_H__ */
