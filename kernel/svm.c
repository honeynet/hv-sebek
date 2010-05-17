/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "types.h"
#include "bitops.h"
#include "string.h"
#include "serial.h"
#include "failure.h"
#include "page.h"
#include "msr.h"
#include "cpufeature.h"
#include "cpu.h"
#include "svm.h"
#include "alloc.h"

/* AMD64 manual Vol. 2, p. 441 */
/* Host save area */
static void *host_save_area;

void * alloc_host_save_area ( void )
{
	void *hsa;

	unsigned long n  = alloc_host_pages ( 1, 1 );
	hsa = (void *) (n << PAGE_SHIFT);

	if (hsa) memset (hsa, 0, PAGE_SIZE);

	return hsa;
}

void __init enable_svm (struct cpuinfo_x86 *c)
{
 	/* Xen does not fill x86_capability words except 0. */
	{
		u32 ecx = cpuid_ecx ( 0x80000001 );
		c->x86_capability[5] = ecx;
	}

	if ( ! ( test_bit ( X86_FEATURE_SVME, &c->x86_capability ) ) ) {
		fatal_failure ("No svm feature!\n");
		return;
	}

	{ /* Before any SVM instruction can be used, EFER.SVME (bit 12
	   * of the EFER MSR register) must be set to 1.
	   * (See AMD64 manual Vol. 2, p. 439) */
		u32 eax, edx;
		rdmsr ( MSR_EFER, eax, edx );
		eax |= EFER_SVME;
		wrmsr ( MSR_EFER, eax, edx );
	}

	outf ("AMD SVM Extension is enabled.\n");

	/* Initialize the Host Save Area */
	// Write host save area address to MSR VM_HSAVE_PA
	{
		u64 phys_hsa;
		u32 phys_hsa_lo, phys_hsa_hi;

		host_save_area = alloc_host_save_area();
		phys_hsa = (u32) host_save_area;
		phys_hsa_lo = (u32) phys_hsa;
		phys_hsa_hi = (u32) (phys_hsa >> 32);
		wrmsr (MSR_K8_VM_HSAVE_PA, phys_hsa_lo, phys_hsa_hi);

		outf ("Host state save area: %x\n", host_save_area);
	}
}
