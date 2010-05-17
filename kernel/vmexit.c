/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "serial.h"
#include "vmexit.h"
#include "vmcb.h"

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
