#ifndef __CPU_H__
#define __CPU_H__

#include "types.h"
#include "cpufeature.h"


enum {
	X86_VENDOR_AMD     = 2,
	X86_VENDOR_UNKNOWN = 0xff
};

struct cpuinfo_x86 {
	u8	x86;		/* CPU family */
	u8	x86_vendor;	/* CPU vendor */
	u8	x86_model;
	u8	x86_mask;
	int	cpuid_max_stdfunc;	/* Maximum supported CPUID standard function, -1 = no CPUID */
	u32	x86_capability[NCAPINTS];
	char	x86_vendor_id[16];
	char	x86_model_id[64];
	int 	x86_cache_size;  /* in KB */
	int	x86_clflush_size;
	int	x86_cache_alignment;
	int	x86_tlbsize;	/* number of 4K pages in DTLB/ITLB combined(in pages)*/
        u8    x86_virt_bits, x86_phys_bits;
	u8	x86_max_cores;	/* cpuid returned max cores value */
        u32   x86_power;
	u32   cpuid_max_exfunc;	/* Max extended CPUID function supported */
	unsigned long loops_per_jiffy;
	u8	apicid;
	u8	booted_cores;	/* number of cores as seen by OS */
};


extern void __init enable_amd_svm ( void );


#endif /* __CPU_H__ */
