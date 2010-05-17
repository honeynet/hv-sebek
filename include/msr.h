#ifndef __MSR_H__
#define __MSR_H__


/* MSR name and MSR address (AMD64 manual vol. 2, pp. 506-509) */
#define MSR_PAT		   0x00000277 /* Page-attribute table (PAT) */
#define MSR_K7_HWCR	   0xc0010015
#define MSR_K8_VM_HSAVE_PA 0xc0010117
#define MSR_EFER 	   0xc0000080 /* Extended feature register */


/* EFER bits (vol. 2, p. 55) */
#define _EFER_SCE   0 /* System Call Extensions */
#define _EFER_LME   8 /* Long mode enable */
#define _EFER_LMA  10 /* Long mode active */
#define _EFER_NX   11 /* No execute enable */
#define _EFER_SVME 12 /* Secure Virtual Machine Enable */
#ifdef __ASSEMBLY__
#  define EFER_SCE   ( 1 << _EFER_SCE )
#  define EFER_LME   ( 1 << _EFER_LME )
#  define EFER_LMA   ( 1 << _EFER_LMA )
#  define EFER_NX    ( 1 << _EFER_NX )
#  define EFER_SVME  ( 1 << _EFER_SVME )
#else /* ! __ASSEMBLY__ */
#  define EFER_SCE   ( 1UL << _EFER_SCE )
#  define EFER_LME   ( 1UL << _EFER_LME )
#  define EFER_LMA   ( 1UL << _EFER_LMA )
#  define EFER_NX    ( 1UL << _EFER_NX )
#  define EFER_SVME  ( 1UL << _EFER_SVME )
#endif /* __ASSEMBLY__ */


#ifndef __ASSEMBLY__

static inline void
cpuid ( int op, unsigned int *eax, unsigned int *ebx,
	unsigned int *ecx, unsigned int *edx )
{
	//TODO: esi get clobbered?
	__asm__("pushl %%ebx; cpuid; movl %%ebx,%%esi; popl %%ebx"
		: "=a" (*eax),
		  "=S" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "0" (op));
}

static inline unsigned int
cpuid_eax(unsigned int op)
{
	unsigned int eax;

	__asm__("pushl %%ebx; cpuid; popl %%ebx"
		: "=a" (eax)
		: "0" (op)
		: "cx", "dx");
	return eax;
}

static inline
unsigned int cpuid_ebx(unsigned int op)
{
	unsigned int eax, ebx;

	//TODO: esi get clobbered?
	__asm__("pushl %%ebx; cpuid; movl %%ebx,%%esi; popl %%ebx"
		: "=a" (eax), "=S" (ebx)
		: "0" (op)
		: "cx", "dx" );
	return ebx;
}

static inline
unsigned int cpuid_ecx(unsigned int op)
{
	unsigned int eax, ecx;

	__asm__("pushl %%ebx; cpuid; popl %%ebx"
		: "=a" (eax), "=c" (ecx)
		: "0" (op)
		: "dx" );
	return ecx;
}

static inline
unsigned int cpuid_edx(unsigned int op)
{
	unsigned int eax, edx;

	__asm__("pushl %%ebx; cpuid; popl %%ebx"
		: "=a" (eax), "=d" (edx)
		: "0" (op)
		: "cx");
	return edx;
}

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

#endif /* __ASSEMBLY__ */


#endif /* __MSR_H__ */
