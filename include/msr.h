#ifndef __MSR_H__
#define __MSR_H__

#include "msr-index.h"
#include "types.h"

#ifndef __ASSEMBLY__

static inline void
rdmsr(unsigned int msr, u32 *low, u32 *high)
{
    __asm__ __volatile__("rdmsr"
			    : "=a" (*low), "=d" (*high)
			    : "c" (msr));
}

static inline void
rdmsrl(unsigned int msr, u64 *val)
{
	u32 low, high;
	rdmsr(msr, &low, &high);
	*val = low | ((u64)high << 32);
}

static inline void
wrmsr(unsigned int msr, u32 low, u32 high)
{
	__asm__ __volatile__("wrmsr"
			  : /* no outputs */
			  : "c" (msr), "a" (low), "d" (high)
			  : "memory");
}

static inline void
wrmsrl(unsigned int msr, u64 val)
{
	u32 low, high;
	low = (u32)val;
	high = (u32)(val >> 32);
	wrmsr(msr, low, high);
}

#endif /* __ASSEMBLY__ */


#endif /* __MSR_H__ */
