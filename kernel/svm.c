/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

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
#include "intercept.h"

#define SVM_FEATURE_NPT            (1 <<  0)
#define SVM_FEATURE_LBRV           (1 <<  1)
#define SVM_FEATURE_SVML           (1 <<  2)
#define SVM_FEATURE_NRIP           (1 <<  3)
#define SVM_FEATURE_PAUSE_FILTER   (1 << 10)

#define NESTED_EXIT_HOST    0   /* Exit handled on host level */
#define NESTED_EXIT_DONE    1   /* Exit caused nested vmexit  */
#define NESTED_EXIT_CONTINUE    2   /* Further checks needed  */ 

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

static struct vmcb * alloc_vmcb ( void )
{
	struct vmcb *vmcb;

	const unsigned long pfn = alloc_host_pages (1, 1);
	//outf("Free page for vmcb: %x\n", pfn);
	vmcb = (struct vmcb *) (pfn << PAGE_SHIFT);
	memset (( char *) vmcb, 0, sizeof (struct vmcb));

	return vmcb;
}

static void set_control_params (struct vmcb *vmcb)
{
	//Note: anything not set will be 0 (since vmcb was filled with 0)

	/****************************** SVM CONFIGURATION *****************************/
	/******************************************************************************/
	/* Enable/disable nested paging (See AMD64 manual Vol. 2, p. 409) */
	vmcb->np_enable = 1;
	outf ("Nested paging enabled.\n");

	// Anh - set this to 1 will make VMRUN to flush all TBL entries, regardless of ASID
	// and global / non global property of pages
    vmcb->tlb_control = 0;

    //Anh - time stamp counter offset, to be added to guest RDTSC and RDTSCP instructions
	/* To be added in RDTSC and RDTSCP */
	vmcb->tsc_offset = 0;

	//Guest address space identifier (ASID), must <> 0 - vol2 373
	vmcb->guest_asid = 1;

	/* Intercept the VMRUN and VMMCALL instructions */
	//must intercept VMRUN at least vol2 373
	vmcb->general2_intercepts = (INTRCPT_VMRUN | INTRCPT_VMMCALL);

	//Anh - allocating a region for IOPM (permission map)
	//and fill it with 0x00 (not intercepting anything)
	vmcb->iopm_base_pa  = create_intercept_table ( 12 << 10 ); /* 12 Kbytes */

	//Anh - allocating a region for msr intercept table, and fill it with 0x00
	vmcb->msrpm_base_pa = create_intercept_table ( 8 << 10 );  /* 8 Kbytes */

	/********** WHAT TO INTERCEPT *********/
	//Note: start without any interception. Specific interception will be enabled
	// by user program when appropriate.

//	vmcb->general1_intercepts |= INTRCPT_INTN;
}

/********************************************************************************************/
/********************************* INITIALIZE VM STATE *************************************/
/********************************************************************************************/

//Anh - Test, to set initial state of VM to state of machine right after power up
static void set_vm_to_powerup_state(struct vmcb * vmcb)
{
	// vol 2 p350
	memset(vmcb, 0, sizeof(vmcb));

	vmcb->cr0 = 0x0000000060000010;
	vmcb->cr2 = 0;
	vmcb->cr3 = 0;
	vmcb->cr4 = 0;
	vmcb->rflags = 0x2;
	vmcb->efer = EFER_SVME; // exception

	vmcb->rip = 0xFFF0;
	vmcb->cs.sel = 0xF000;
	vmcb->cs.base = 0xFFFF0000;
	vmcb->cs.limit = 0xFFFF;

	vmcb->ds.sel = 0;
	vmcb->ds.limit = 0xFFFF;
	vmcb->es.sel = 0;
	vmcb->es.limit = 0xFFFF;
	vmcb->fs.sel = 0;
	vmcb->fs.limit = 0xFFFF;
	vmcb->gs.sel = 0;
	vmcb->gs.limit = 0xFFFF;
	vmcb->ss.sel = 0;
	vmcb->ss.limit = 0xFFFF;

	vmcb->gdtr.base = 0;
	vmcb->gdtr.limit = 0xFFFF;
	vmcb->idtr.base = 0;
	vmcb->idtr.limit = 0xFFFF;

	vmcb->ldtr.sel = 0;
	vmcb->ldtr.base = 0;
	vmcb->ldtr.limit = 0xFFFF;
	vmcb->tr.sel = 0;
	vmcb->tr.base = 0;
	vmcb->tr.limit = 0xFFFF;

//	vmcb->rdx = model info;
//	vmcb->cr8 = 0;
}

static void set_vm_to_mbr_start_state(struct vmcb* vmcb)
{
	// Prepare to load GRUB for the second time
	// Basically copy the state when GRUB is first started
	// Note: some other states will be set in svm_asm.S, at load_guest_states:
	// ebx, ecx, edx, esi, edi, ebp

	//memset(vmcb, 0, sizeof(vmcb));

	vmcb->rax = 0;

	vmcb->rip = 0x7c00;

	vmcb->cs.attrs.bytes = 0x019B;
	vmcb->cs.limit = 0xFFFF;
	vmcb->cs.base = 0;
	vmcb->cs.sel = 0;

	vmcb->ds.sel=0x0040;
	vmcb->fs.sel=0xE717;
	vmcb->gs.sel=0xF000;

	int i;
	struct seg_selector *segregs [] = {&vmcb->ss, &vmcb->ds, &vmcb->es, &vmcb->fs, &vmcb->gs, NULL};
	for (i = 0; segregs [i] != NULL; i++)
	{
			struct seg_selector * x = segregs [i];
			x->attrs.bytes = 0x93;
			x->base = 0;
			x->limit = 0xFFFF;
	}

	vmcb->rsp=0x000003E2;

	vmcb->ss.attrs.bytes = 0x193;
	vmcb->ds.base = 00000400;
	vmcb->fs.base = 0xE7170;
	vmcb->gs.base = 0xF0000;

	vmcb->efer = EFER_SVME;	// must be set - vol2 p373
	// EFLAGS=odItszaPc;

	vmcb->cr0 = 0x0000000000000010;

	vmcb->idtr.limit = 0x3FF;
	vmcb->idtr.base = 0;
	//setup idt?

	vmcb->gdtr.limit = 0x20;
	vmcb->gdtr.base = 0x06E127;
	//setup gdt

	vmcb->rflags = 0x2206;

	vmcb->cpl = 0; /* must be equal to SS.DPL - TODO */

	/*Anh - Setup PAT, vol2 p193
	 * Each page table entry use 3 flags: PAT PCD PWT to specify index of the
	 * corresponding PAT entry, which then specify the type of memory access for that page
		PA0 = 110	- Writeback
		PA1 = 100	- Writethrough
		PA2 = 111	- UC-
		PA3 = 000	- Unchachable
		PA4 = 110	- Writeback
		PA5 = 100	- Writethrough
		PA6 = 111	- UC-
		PA7 = 000	- Unchachable
	 This is also the default PAT */
	vmcb->g_pat = 0x7040600070406ULL;

	/******* GUEST INITIAL OPERATING MODE  ***************/
	/******* pick one ******/

	/* Legacy real mode
	vmcb->cr0 = X86_CR0_ET;
	vmcb->cr4 = 0;
	*/

	/* Legacy protected mode, paging disabled
	vmcb->cr0 = X86_CR0_PE | X86_CR0_ET;
	vmcb->cr3 = 0;
	vmcb->cr4 = 0;
	*/

	/* Legacy protected mode, paging enabled (4MB pages)
	vmcb->cr0 = X86_CR0_PE | X86_CR0_ET | X86_CR0_PG;
	vmcb->cr3 = 0x07000000; //Anh -vmcb->cr3 must be aligned by page size (4MB)???
	vmcb->cr4 = X86_CR4_PSE; //Anh - enable 4MB pages
	*/

	/*//Anh - Long mode
	vmcb->cr0 = X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_PG;
	vmcb->cr4 = X86_CR4_PAE;
	vmcb->cr3 = 0x07000000; // dummy
	vmcb->efer |= EFER_LME | EFER_LMA;
	*/
}

static void svm_run ( struct vm_info *vm )
{
//	outf(">>>>>>>>>>> GOING INTO THE MATRIX!!!!!\n");
	//u64 p_vmcb = vm->vmcb;

	/* Set the pointer to VMCB to %rax (vol. 2, p. 440) */
	__asm__("pushq %%rax; movq %0, %%rax" :: "r" (vm->vmcb));

	svm_launch ();

	__asm__("popq %rax");
}

static void enable_svm (struct cpuinfo_x86 *c)
{
 	/* Xen does not fill x86_capability words except 0. */
	{
		u32 ecx = cpuid_ecx ( 0x80000001 );
		c->x86_capability[5] = ecx;
	}

	if ( ! ( test_bit ( X86_FEATURE_SVM, &c->x86_capability ) ) ) {
		fatal_failure ("No svm feature!\n");
		return;
	}

	/* Test if Nested Paging is supported */
	unsigned int np = cpuid_edx(0x8000000a) & SVM_FEATURE_NPT;
	if ( ! np ) {
		fatal_failure ("Nested paging is not supported.\n");
		return;
	}
	
	{ /* Before any SVM instruction can be used, EFER.SVME (bit 12
	   * of the EFER MSR register) must be set to 1.
	   * (See AMD64 manual Vol. 2, p. 439) */
		u32 eax, edx;
		rdmsr ( MSR_EFER, &eax, &edx );
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
		phys_hsa = (u64) host_save_area;
		phys_hsa_lo = (u32) phys_hsa;
		phys_hsa_hi = (u32) (phys_hsa >> 32);
		wrmsr (MSR_K8_VM_HSAVE_PA, phys_hsa_lo, phys_hsa_hi);

		outf ("Host state save area: %x\n", host_save_area);
	}

}

void init_amd (struct vm_info *vm, struct cpuinfo_x86 *cpuinfo)
{
	//TODO: uncomment - check why it does not work on HP tx2500
//	if (cpuinfo->x86 != 0xf) fatal_failure("Failed in init_amd\n");

	/* Bit 31 in normal CPUID used for nonstandard 3DNow ID;
	   3DNow is IDd by bit 31 in extended CPUID (1*32+31) anyway */
	clear_bit (0*32+31, &cpuinfo->x86_capability);

	/* On C+ stepping K8 rep microcode works well for copy/memset */
	unsigned level = cpuid_eax (1);
	if ( (level >= 0x0f48 && level < 0x0f50) || level >= 0x0f58)
		set_bit (X86_FEATURE_REP_GOOD, &cpuinfo->x86_capability);

	/* Enable workaround for FXSAVE leak */
	set_bit (X86_FEATURE_FXSAVE_LEAK, &cpuinfo->x86_capability);

	display_cacheinfo (cpuinfo);

	//Anh - enable SVM, allocate a page for host state save area,
	// and copy its address into MSR_K8_VM_HSAVE_PA MSR
	enable_svm (cpuinfo);
	
	// Allocate a new page inside host memory for storing VMCB.
	struct vmcb *vmcb;
	vmcb = alloc_vmcb();
	vm->vmcb = vmcb;
	outf("VMCB location: %x\n", vmcb);
	
	/* Set Host-level CR3 to use for nested paging.  */
	vmcb->n_cr3 = vm->n_cr3;

	//Set control params for this VM
	//such as whether to enable nested paging, what to intercept...
	set_control_params (vm->vmcb);
	
	// Set VM initial state
    // Guest VM start at MBR code, which is GRUB stage 1
    // vmcb->rip = 0x7c00; address of loaded MBR
    set_vm_to_mbr_start_state(vm->vmcb);
    
    vm->vm_run = svm_run;
    vm->vm_exit = handle_vmexit;
}
