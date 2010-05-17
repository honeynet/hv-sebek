#ifndef __SYSTEM_H__
#define __SYSTEM_H__

//#include "vmcb.h"

/* [REF] AMD64 manual vol. 2, p. 59 */
#define _X86_CR4_PSE 4 /* Page Size Extensions */
#define _X86_CR4_PAE 5 /* Physical-Address Extension */
#define _X86_CR4_PGE 7 /* Page-Global Enable */
#ifdef __ASSEMBLY__
#  define X86_CR4_PSE  ( 1 << _X86_CR4_PSE )
#  define X86_CR4_PAE  ( 1 << _X86_CR4_PAE )
#  define X86_CR4_PGE  ( 1 << _X86_CR4_PGE )
#else /* ! __ASSEMBLY__ */
#  define X86_CR4_PSE  ( 1UL << _X86_CR4_PSE )
#  define X86_CR4_PAE  ( 1UL << _X86_CR4_PAE )
#  define X86_CR4_PGE  ( 1UL << _X86_CR4_PGE )
#endif /* __ASSEMBLY__ */

/* [REF] AMD64 manual vol. 2, p. 53 */
#define _X86_CR0_PE  0 /* Protection enabled */
#define _X86_CR0_MP  1 /* Monitor coprocessor */
#define _X86_CR0_ET  4 /* Extension type */
#define _X86_CR0_NE  5 /* Numeric error */
#define _X86_CR0_WP 16 /* Write protected */
#define _X86_CR0_AM 18 /* Aligned mask */
#define _X86_CR0_NW 29 /* Not Writethrough */
#define _X86_CR0_CD 30 /* Cache Disable */
#define _X86_CR0_PG 31 /* Paging */
#ifdef __ASSEMBLY__
#  define X86_CR0_PE  ( 1 << _X86_CR0_PE )
#  define X86_CR0_MP  ( 1 << _X86_CR0_MP )
#  define X86_CR0_ET  ( 1 << _X86_CR0_ET )
#  define X86_CR0_NE  ( 1 << _X86_CR0_NE )
#  define X86_CR0_WP  ( 1 << _X86_CR0_WP )
#  define X86_CR0_AM  ( 1 << _X86_CR0_AM )
#  define X86_CR0_NW  ( 1 << _X86_CR0_NW )
#  define X86_CR0_CD  ( 1 << _X86_CR0_CD )
#  define X86_CR0_PG  ( 1 << _X86_CR0_PG )
#else /* ! __ASSEMBLY__ */
#  define X86_CR0_PE  ( 1UL << _X86_CR0_PE )
#  define X86_CR0_MP  ( 1UL << _X86_CR0_MP )
#  define X86_CR0_ET  ( 1UL << _X86_CR0_ET )
#  define X86_CR0_NE  ( 1UL << _X86_CR0_NE )
#  define X86_CR0_WP  ( 1UL << _X86_CR0_WP )
#  define X86_CR0_AM  ( 1UL << _X86_CR0_AM )
#  define X86_CR0_NW  ( 1UL << _X86_CR0_NW )
#  define X86_CR0_CD  ( 1UL << _X86_CR0_CD )
#  define X86_CR0_PG  ( 1UL << _X86_CR0_PG )

#  define X86_RFLAGS_TF  	( 1UL << 8 )
#  define X86_DR6_BS  		( 1UL << 14 )
#endif /* __ASSEMBLY__ */

#ifndef __ASSEMBLY__

static inline unsigned long
read_cr0 ( void )
{
	unsigned long cr0;
	asm volatile ( "movq %%cr0, %0" : "=r" ( cr0 ) );
	return cr0;
}

static inline unsigned long
read_cr3 ( void )
{
	unsigned long cr3;
	asm ("movq %%cr3, %0" : "=r" ( cr3 ) );
	return cr3;
}

static inline unsigned long
read_cr4 ( void )
{
	unsigned long cr4;
	asm ( "movq %%cr4, %0" : "=r" ( cr4 ) );
	return cr4;
}

static inline void
write_cr0 ( unsigned long val )
{
	asm volatile ( "movq %0, %%cr0" :: "r" ( val ) );
}

static inline void
write_cr3 ( unsigned long val )
{
	asm volatile ( "movq %0, %%cr3" :: "r" ( val ) );
}

static inline void
write_cr4 ( unsigned long val )
{
	asm volatile ( "movq %0, %%cr4" :: "r" ( val ) );
}

/* Interrupt Descriptor Table Register (IDTR)
 *
 * The content of the IDTR is copied by sidt to the six
 * bytes of memory specified by the operand. The first word
 * at the effective address is assigned the LIMIT field of
 * the register. If the operand-size attribute is 16-bit,
 * the 32-bit BASE field of the register is assigned to the
 * next four bytes.
 *
 * MSB of the IDT base address of a native system: 0xc0
 */
static inline unsigned long get_idt_base (void)
{
	unsigned char	idtr[6];
	unsigned long	idt;

	asm volatile ("sidt %0" : "=m" (idtr));
	idt = *((unsigned long *) &idtr[2]);

	return (idt);
}

static inline unsigned short get_idt_limit (void)
{
	unsigned char	idtr[6];
	unsigned long	idt;

	asm volatile ("sidt %0" : "=m" (idtr));
	idt = *((unsigned short *) &idtr[0]);

	return (idt);
}

#endif /* __ASSEMBLY__ */

#endif /* __SYSTEM_H__ */
