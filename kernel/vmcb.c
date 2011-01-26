/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#include "serial.h"
#include "failure.h"
#include "msr.h"
#include "vm.h"
#include "vmcb.h"
#include "svm.h"
#include "page.h"
#include "intercept.h"

void print_vmcb_state (struct vmcb *vmcb)
{
	outf ( "*******  VMCB STATE  *********\n" );
	outf ( "cs:ip = %x:%x\n", vmcb->cs.sel, vmcb->rip );
	outf ( "ss:sp = %x:%x\n", vmcb->ss.sel, vmcb->rsp );
	outf ( "ds:bp = %x:%x\n", vmcb->ds.sel, g_rbp);

	outf ( "eax = %x, ebx = %x, ecx = %x, edx = %x\n", vmcb->rax, g_rbx, g_rcx, g_rdx);
	outf ( "esi = %x, edi = %x", g_rsi, g_rdi);

	outf ( "cpl=%x\n", vmcb->cpl );
	outf ( "cr0=%x, cr3=%x, cr4=%x\n", vmcb->cr0, vmcb->cr3, vmcb->cr4 );
	outf ( "rflags=%x, efer=%x\n", vmcb->rflags, vmcb->efer );

//	outf ( "cs.attrs=%x, ds.attrs=%x\n", vmcb->cs.attrs.bytes, vmcb->ds.attrs.bytes );
//	outf ( "cs.base=%x, ds.base=%x\n", vmcb->cs.base, vmcb->ds.base );
//	outf ( "cs.limit=%x, ds.limit=%x\n", vmcb->cs.limit, vmcb->ds.limit);
}

#define BIT_MASK(n)  ( ~ ( ~0ULL << (n) ) )
#define SUB_BIT(x, start, len) ( ( ( ( x ) >> ( start ) ) & BIT_MASK ( len ) ) )


/* [REF] AMD64 manual vol 2, pp. 373 */
static int check_efer_svme ( const struct vmcb *vmcb )
{
	return ( ! ( vmcb->efer & EFER_SVME ) );
}

static int check_cr0cd_cr0nw ( const struct vmcb *vmcb )
{
	return ( ( ! ( vmcb->cr0 & X86_CR0_CD ) ) && ( vmcb->cr0 & X86_CR0_NW ) );
}

static int check_cr0_32_63 ( const struct vmcb *vmcb )
{
	return ( SUB_BIT ( vmcb->cr0, 32,  32 ) );
}

static int check_cr4_11_63 ( const struct vmcb *vmcb )
{
	return ( SUB_BIT ( vmcb->cr4, 11, (u64) 53 ) );
}

static int check_dr6_32_63 ( const struct vmcb *vmcb )
{
	return ( SUB_BIT ( vmcb->dr6, 32, 32 ) );
}

static int check_dr7_32_63 ( const struct vmcb *vmcb )
{
	return ( SUB_BIT ( vmcb->dr7, 32, 32 ) );
}

static int check_efer_15_63 ( const struct vmcb *vmcb )
{
	return ( SUB_BIT ( vmcb->efer, 15, 49 ) );
}

static int check_eferlme_cr0pg_cr4pae ( const struct vmcb *vmcb )
{
	return ( ( vmcb->efer & EFER_LME ) && ( vmcb->cr0 & X86_CR0_PG ) &&
		 ( ! ( vmcb->cr4 & X86_CR4_PAE ) ) );
}

static int check_eferlme_cr0pg_cr0pe ( const struct vmcb *vmcb )
{
	return ( ( vmcb->efer & EFER_LME ) && ( vmcb->cr0 & X86_CR0_PG ) &&
		 ( ! ( vmcb->cr0 & X86_CR0_PE ) ) );
}

/* [REF] Code-Segment Register - Long mode */
static int check_eferlme_cr0pg_cr4pae_csl_csd ( const struct vmcb *vmcb )
{
	return ( ( vmcb->efer & EFER_LME ) && ( vmcb->cr0 & X86_CR0_PG ) &&
		 ( vmcb->cr4 & X86_CR4_PAE ) && ( vmcb->cs.attrs.fields.l ) && ( vmcb->cs.attrs.fields.db ) );
}

static int check_vmrun_intercept ( const struct vmcb *vmcb )
{
	return ( ! ( vmcb->general2_intercepts & INTRCPT_VMRUN ) );
}

// TODO: HeeDong - Fill the function
static int check_msr_ioio_intercept_tables ( const struct vmcb *vmcb )
{
	/* The MSR or IOIO intercept tables extend to a physical address >= the maximum supported physical address */
//	return ( ( vmcb->iopm_base_pa >= ) || ( vmcb->msrpm_base_pa >= ) ); /* [DEBUG] */
	return 0;
}

#if 0
static int
check_misc ( const struct vmcb *vmcb )
{

//	if ( vmcb->cr3 ) { /* Any MBZ (must be zero) bits of CR3 are set */
//		0-2, 5-11		(legacy, non-pae)
//		0-2			(legacy, pae)
//		0-2, 5-11, and 52-63	(long, non-pae)
//	}

	/* Other MBZ bits exist in various registers stored in the VMCB. */


	/* Illegal event injection (vol. 2, p. 468) */

	return 0;
}
#endif

struct consistencty_check {
	int ( *func ) ( const struct vmcb *vmcb );
	char *error_msg;
};

void vmcb_check_consistency ( struct vmcb *vmcb )
{
	const struct consistencty_check tbl[]
		= { { &check_efer_svme,   "EFER.SVME is not set.\n" },
		    { &check_cr0cd_cr0nw, "CR0.CD is not set, and CR0.NW is set.\n" },
		    { &check_cr0_32_63,   "CR0[32:63] are not zero.\n" },
		    { &check_cr4_11_63,   "CR4[11:63] are not zero.\n" },
		    { &check_dr6_32_63,   "DR6[32:63] are not zero.\n" },
		    { &check_dr7_32_63,   "DR7[32:63] are not zero.\n" },
		    { &check_efer_15_63,  "EFER[15:63] are not zero.\n" },
		    { &check_eferlme_cr0pg_cr4pae, "EFER.LME is set, CR0.PG is set, and CR4.PAE is not set.\n" },
		    { &check_eferlme_cr0pg_cr0pe,  "EFER.LME is set, CR0.PG is set, and CR4.PE is not set.\n" },
		    { &check_eferlme_cr0pg_cr4pae_csl_csd, "EFER.LME, CR0.PG, CR4.PAE, CS.L, and CS.D are set.\n" },
		    { &check_vmrun_intercept, "The VMRUN intercept bit is clear.\n" },
		    { &check_msr_ioio_intercept_tables, "Wrong The MSR or IOIO intercept tables address.\n" } };
	const size_t nelm = sizeof ( tbl ) / sizeof ( struct consistencty_check );

	int i;
	for (i = 0; i < nelm; i++)
	{
		if ( (* tbl[i].func) (vmcb) )
		{
			outf (tbl[i].error_msg);
			fatal_failure ("Consistency check failed.\n");
		}
	}
}

/********************************************************************************************/

static void seg_selector_dump ( char *name, const struct seg_selector *s )
{
	outf ( "%s: sel=%x, attr=%x, limit=%x, base=%x\n",
		 name, s->sel, s->attrs.bytes, s->limit,
		 (unsigned long long)s->base );
}

void vmcb_dump( const struct vmcb *vmcb )
{
	outf("Dumping guest's current state\n");
	outf("Size of VMCB = %x, address = %x\n",
	       (int) sizeof(struct vmcb), vmcb);

	outf("cr_intercepts = %x dr_intercepts = %x exception_intercepts = %x\n",
	       vmcb->cr_intercepts, vmcb->dr_intercepts, vmcb->exception_intercepts);
	outf("general1_intercepts = %x general2_intercepts = %x\n",
	       vmcb->general1_intercepts, vmcb->general2_intercepts);
	outf("iopm_base_pa = %x msrpm_base_pa = %x tsc_offset = %x\n",
	       (unsigned long long) vmcb->iopm_base_pa,
	       (unsigned long long) vmcb->msrpm_base_pa,
	       (unsigned long long) vmcb->tsc_offset);
	outf("tlb_control = %x vintr = %x interrupt_shadow = %x\n", vmcb->tlb_control,
	       (unsigned long long) vmcb->vintr.bytes,
	       (unsigned long long) vmcb->interrupt_shadow);
	outf("exitcode = %x exitintinfo = %x\n",
	       (unsigned long long) vmcb->exitcode,
	       (unsigned long long) vmcb->exitintinfo.bytes);
	outf("exitinfo1 = %x exitinfo2 = %x \n",
	       (unsigned long long) vmcb->exitinfo1,
	       (unsigned long long) vmcb->exitinfo2);
	outf("np_enable = %x guest_asid = %x\n",
	       (unsigned long long) vmcb->np_enable, vmcb->guest_asid);
	outf("cpl = %x efer = %x star = %x lstar = %x\n",
	       vmcb->cpl, (unsigned long long) vmcb->efer,
	       (unsigned long long) vmcb->star, (unsigned long long) vmcb->lstar);
	outf("CR0 = %x CR2 = %x\n",
	       (unsigned long long) vmcb->cr0, (unsigned long long) vmcb->cr2);
	outf("CR3 = %x CR4 = %x\n",
	       (unsigned long long) vmcb->cr3, (unsigned long long) vmcb->cr4);
	outf("RSP = %x  RIP = %x\n",
	       (unsigned long long) vmcb->rsp, (unsigned long long) vmcb->rip);
	outf("RAX = %x  RFLAGS=%x\n",
	       (unsigned long long) vmcb->rax, (unsigned long long) vmcb->rflags);
	outf("DR6 = %x, DR7 = %x\n",
	       (unsigned long long) vmcb->dr6, (unsigned long long) vmcb->dr7);
	outf("CSTAR = %x SFMask = %x\n",
	       (unsigned long long) vmcb->cstar, (unsigned long long) vmcb->sfmask);
	outf("KernGSBase = %x PAT = %x \n",
	       (unsigned long long) vmcb->kerngsbase,
	       (unsigned long long) vmcb->g_pat);

	/* print out all the selectors */
	seg_selector_dump ( "CS", &vmcb->cs );
	seg_selector_dump ( "DS", &vmcb->ds );
	seg_selector_dump ( "SS", &vmcb->ss );
	seg_selector_dump ( "ES", &vmcb->es );
	seg_selector_dump ( "FS", &vmcb->fs );
	seg_selector_dump ( "GS", &vmcb->gs );
	seg_selector_dump ( "GDTR", &vmcb->gdtr );
	seg_selector_dump ( "LDTR", &vmcb->ldtr );
	seg_selector_dump ( "IDTR", &vmcb->idtr );
	seg_selector_dump ( "TR", &vmcb->tr );
}

void print_vmexit_exitcode (struct vmcb * vmcb)
{
	outf ( "#VMEXIT: ");

	switch ( vmcb->exitcode ) {
	case VMEXIT_EXCEPTION_PF:
		outf ( "EXCP (page fault)" ); break;
	case VMEXIT_NPF:
		outf ( "NPF (nested-paging: host-level page fault)" ); break;
	case VMEXIT_INVALID:
		outf ( "INVALID" ); break;
	default:
		outf ( "%x", ( unsigned long ) vmcb->exitcode ); break;
	}

	outf ( "\n" );
	outf ( "exitinfo1 (error_code) = %x, ", vmcb->exitinfo1);
	outf ( "exitinfo2 = %x, ", vmcb->exitinfo2);
	outf ( "exitINTinfo = %x\n", vmcb->exitintinfo );
}

/*****************************************************/
//manual vol 2 - 8.4.2 Page-Fault Error Code
// Note for NPF: p410 - 15.24.6 Nested versus Guest Page Faults, Fault Ordering
void print_page_errorcode(u64 errcode)
{
	if (errcode & 1) outf ( "page fault was caused by a page-protection violation\n" );
	else outf ( "page fault was caused by a not-present page\n" );

	if (errcode & 2) outf ( "memory access was write\n" );
	else outf ( "memory access was read\n" );

	if (errcode & 4 ) outf ( "an access in user mode caused the page fault\n" );
	else outf ( "an access in supervisor mode caused the page fault\n" );

	if (errcode & 8 ) outf ( "error caused by reading a '1' from reserved field, \
			when CR4.PSE=1 or CR4.PAE=1\n" );

	if (errcode & 16 ) outf ( "error caused by instruction fetch, when \
			EFER.NXE=1 && CR4.PAE=1");
}
