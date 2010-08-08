/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#include "cpu.h"
#include "string.h"
#include "failure.h"
#include "page.h"
#include "msr.h"
#include "vm.h"
#include "svm.h"
#include "vmx.h"
#include "alloc.h"
#include "vmm.h"
#include "intercept.h"
#include "serial.h"
#include "types.h"
#include "user.h"

unsigned long create_intercept_table ( unsigned long size )
{
	const unsigned long pfn = alloc_host_pages ( size >> PAGE_SHIFT, 1 );
	void *p = ( void * ) ( pfn << PAGE_SHIFT );

	/* vol. 2, p. 445 */
	//memset ( p, 0xff, size );
	memset ( p, 0x00, size );

	return pfn << PAGE_SHIFT;
}

/* Anh - Create the nested paging table that map guest physical to host physical
 * return the (host) physical base address of the table.
 * Note: Nested paging table must use the same paging mode as the host,
 * regardless of guest paging mode - See AMD man vol2:
 * The extra translation uses the same paging mode as the VMM used when it executed the
 * most recent VMRUN.
 *
 * Also, it is important to note that gCR3 and the guest page table entries contain
 * guest physical addresses, not system physical addresses.
 * Hence, before accessing a guest page table entry, the table walker first
 * translates that entry’s guest physical address into a system physical address.*/

static unsigned long create_4mb_nested_pagetable (unsigned long vmm_pmem_start, unsigned long vmm_pmem_size,
		struct e820_map *e820 )
{
	const unsigned long cr3  = pg_table_alloc();

	//const unsigned long vm_pmem_pfn = PFN_DOWN_2MB ( PHYS ( vm_pmem_start ) );
	int i;

	unsigned long vmm_pagestart = PAGE_DOWN_4MB(vmm_pmem_start);

	//vmm_pageend = page after VMM - 1
	unsigned long vmm_pageend = PAGE_UP_4MB(vmm_pmem_start + vmm_pmem_size) - 1;

	outf( "VMM start = %x; VMM end = %x.\n", vmm_pagestart, vmm_pageend);

	//Map all 4GB except VMM region (2^10 * 4MB)
	unsigned long all4gb = 1 << 10;
	for ( i = 0; i < all4gb; i++ )
	{
		const unsigned long guest_paddr = i << PAGE_SHIFT_4MB;

		// enable this to hide VMM memory, a #NP will be created when the
		// guest tries to access this region
		if ((guest_paddr >= vmm_pagestart) && (guest_paddr <= vmm_pageend)) continue;
		if ((guest_paddr + PAGE_SIZE_4MB >= vmm_pagestart)
				&& (guest_paddr + PAGE_SIZE_4MB <= vmm_pageend)) continue;

		// identity map
		mmap_4mb (cr3, guest_paddr, guest_paddr, -1 /*using default flags*/ );
	}

	outf( "Nested page table created.\n" );

//	print_4MB_pg_table(cr3);

	return cr3;
}

static unsigned long create_2mb_nested_pagetable (unsigned long vmm_pmem_start, unsigned long vmm_pmem_size,
		struct e820_map *e820 )
{
	const unsigned long cr3  = pml4_table_alloc();

	int i;

	unsigned long vmm_pagestart = PAGE_DOWN_2MB(vmm_pmem_start);

	//vmm_pageend = page after VMM - 1
	unsigned long vmm_pageend = PAGE_UP_2MB(vmm_pmem_start + vmm_pmem_size) - 1;

	outf( "VMM start = %x; VMM end = %x.\n", vmm_pagestart, vmm_pageend);

	//Map all 4GB except VMM region (2^10 * 4MB)
	unsigned long all4gb = 1 << 10;
	for ( i = 0; i < all4gb; i++ )
	{
		const unsigned long guest_paddr = i << PAGE_SHIFT_2MB;

		// enable this to hide VMM memory, a #NP will be created when the
		// guest tries to access this region
		if ((guest_paddr >= vmm_pagestart) && (guest_paddr <= vmm_pageend)) continue;
		if ((guest_paddr + PAGE_SIZE_2MB >= vmm_pagestart)
				&& (guest_paddr + PAGE_SIZE_4MB <= vmm_pageend)) continue;

		// identity map
		mmap_pml4 (cr3, guest_paddr, guest_paddr, 1 /* user */ );
	}

	outf( "Nested page table created.\n" );

//	print_4MB_pg_table(cr3);

	return cr3;
}

/********************************************************************************************/
/******************************* NESTED PAGING PROTECTION ***********************************/
/********************************************************************************************/

void set_4mb_pagetable_attr (u32 cr3, u32 linearaddr, u16 attr)
{
	// identity map
	mmap_4mb (cr3, linearaddr, linearaddr, attr);
}

u16 get_4mb_pagetable_attr (u64 cr3, u64 linearaddr)
{
	int index = linearaddr >> PAGE_SHIFT_4MB;
	struct pd4M_entry * entry = (struct pd4M_entry *) (cr3 + index * sizeof(struct pd4M_entry));

	return entry->flags;
}

void __vm_protect_all_nonPAE_page(u64 cr3)
{
	int index;
	unsigned long all4gb = 1 << 10;
	for ( index = 0; index < all4gb; index++ )
	{
		int * entry = (int *) (cr3 + index * 4);
		*entry &= 0xFFFFFFFF - PTTEF_RW;
	}
}
void vm_protect_all_nonPAE_nestedpage(struct vm_info * vm)
{
	outf("protect all pages\n");
	__vm_protect_all_nonPAE_page(vm->n_cr3);
}

void vm_protect_all_nonPAE_guestpage(struct vm_info * vm)
{
	outf("protect all GUEST pages\n");
	__vm_protect_all_nonPAE_page(vm->vmcb->cr3);
}

void __vm_unprotect_nonPAE_page(u64 cr3)
{
	int index;

	unsigned long all4gb = 1 << 10;
	for ( index = 0; index < all4gb; index++ )
	{
		int * entry = (int *) (cr3 + index * 4);
		*entry |= PTTEF_RW;
	}
}

void vm_unprotect_all_nonPAE_nestedpage(struct vm_info * vm)
{
	outf("unprotect all pages\n");
	__vm_unprotect_nonPAE_page(vm->n_cr3);
}

void vm_unprotect_all_nonPAE_guestpage(struct vm_info * vm)
{
	outf("unprotect all GUEST pages\n");
	__vm_unprotect_nonPAE_page(vm->vmcb->cr3);
}

/*****************************************************************************************/
/************************ HANDLING VM INTERCEPTS ***************************************/
/*****************************************************************************************/

void vm_disable_intercept(struct vm_info *vm, int flags)
{
//	outf("vm_disable_intercept - %x\n", flags);

	if (flags & USER_UNPACK) {
//		vm->vmcb->rflags &= ~X86_RFLAGS_TF;
//		vm->vmcb->exception_intercepts &= ~INTRCPT_DB;
		vm->vmcb->exception_intercepts &= ~INTRCPT_PF;
		vm_unprotect_all_nonPAE_guestpage(vm);
	}

	//disable taskswitch interception
	if (flags & USER_ITC_TASKSWITCH) {
//		outf("Disable taskswitch interception\n");
		vm->vmcb->cr_intercepts &= ~INTRCPT_WRITE_CR3;
	}

	if (flags & USER_ITC_SWINT) {
//		outf("Disable software interrupt interception\n");
		vm->vmcb->general1_intercepts &= ~INTRCPT_INTN;
	}

	if (flags & USER_ITC_IRET) {
//		outf("Enable software interrupt interception\n");
		vm->vmcb->general1_intercepts &= ~INTRCPT_IRET;
	}

	if (flags & USER_ITC_SYSCALL) {
//		outf("Disable syscall interception\n");
		/**************** int 80h ***************************/
		vm->vmcb->general1_intercepts &= ~INTRCPT_INTN;

		/**************** sysenter ***************************/
//		vm->vmcb->general1_intercepts &= ~INTRCPT_MSR;
//
//		// Disable R/W interception for sysenter_cs, sysenter_esp and sysenter_eip
//		// each register <=> 2 bits
//		// flags for 3 consecutive registers = 1111 1100 b = 0xFC
//		u8 * sysenter_msrs = vm->vmcb->msrpm_base_pa + MSR_SYSENTER_CS / 4;
//		*sysenter_msrs &= ~0x3;
//
//		vm->vmcb->sysenter_cs = vm->org_sysenter_cs;
////		vm->vmcb->sysenter_esp = vm->org_sysenter_esp;
////		vm->vmcb->sysenter_eip = vm->org_sysenter_eip;
//
//		//Disable interception of that fault
//		vm->vmcb->exception_intercepts &= ~INTRCPT_GP;
	}

	// disable single stepping
	if (flags & USER_SINGLE_STEPPING) {
//		outf("Disable single stepping\n");
		vm->vmcb->rflags &= ~X86_RFLAGS_TF;
		vm->vmcb->exception_intercepts &= ~INTRCPT_DB;
	}
}

void vm_enable_intercept(struct vm_info * vm, int flags)
{
//	outf("vm_enable_intercept - %x\n", flags);

	if (flags & USER_UNPACK) {
//		vm->vmcb->rflags |= X86_RFLAGS_TF;
//		vm->vmcb->exception_intercepts |= INTRCPT_DB;
		vm->vmcb->exception_intercepts |= INTRCPT_PF;
		vm_protect_all_nonPAE_guestpage(vm);
	}

	//enable taskswitch interception
	if (flags & USER_ITC_TASKSWITCH) {
//		outf("Enable taskswitch interception\n");
		vm->vmcb->cr_intercepts |= INTRCPT_WRITE_CR3;

		//vm->vmcb->general1_intercepts |= INTRCPT_TASKSWITCH; <== does not work
		//vm->vmcb->general1_intercepts |= INTRCPT_READTR; <== does not work
	}

	//enable software interrupt interception
	if (flags & USER_ITC_SWINT) {
//		outf("Enable software interrupt interception\n");
		vm->vmcb->general1_intercepts |= INTRCPT_INTN;
	}

	if (flags & USER_ITC_IRET) {
//		outf("Enable software interrupt interception\n");
		vm->vmcb->general1_intercepts |= INTRCPT_IRET;
	}

	//enable syscall interception - both int80 and sys_enter
	if (flags & USER_ITC_SYSCALL) {
//		outf("Enable syscall interception\n");

		/**************** int 80h ***************************/
		vm->vmcb->general1_intercepts |= INTRCPT_INTN;

		/**************** sysenter ***************************/
//		vm->vmcb->general1_intercepts |= INTRCPT_MSR;
//
//		// Intercept R/W to sysenter_cs, sysenter_esp and sysenter_eip
//		// each register <=> 2 bits
//		// flags for 3 consecutive registers = 1111 1100 b = 0xFC
//		u8 * sysenter_msrs = vm->vmcb->msrpm_base_pa + MSR_SYSENTER_CS / 4;
//		*sysenter_msrs |= 0x3;
//
//		vm->org_sysenter_cs = vm->vmcb->sysenter_cs;
////		vm->org_sysenter_esp = vm->vmcb->sysenter_esp;
////		vm->org_sysenter_eip = vm->vmcb->sysenter_eip;
//
//		// Set vmcb's msr values so that syscall will create fault
//		vm->vmcb->sysenter_cs = SYSENTER_CS_FAULT;
////		vm->vmcb->sysenter_esp = SYSENTER_ESP_FAULT;
////		vm->vmcb->sysenter_eip = SYSENTER_EIP_FAULT;
//
//		//Enable interception of that fault
//		vm->vmcb->exception_intercepts |= INTRCPT_GP;	//general protection
//
//		//HeeDong - Set the bit in MSR Permission Map to intercept R/W to STAR MSR
////		u64 * star_msr_set_addr = vmcb->msrpm_base_pa + 0x820;
////		*star_msr_set_addr = *star_msr_set_addr | 0x04;
	}

	// enable single stepping
	if (flags & USER_SINGLE_STEPPING) {
//		outf("Enable single stepping\n");
		vm->vmcb->rflags |= X86_RFLAGS_TF;
		vm->vmcb->exception_intercepts |= INTRCPT_DB;
	}
}

void initialize_fid2name_map(struct vm_info *vm)
{
	vm->nOpenFile = 0;

	const unsigned long pfn = alloc_host_pages (1, 1);
	vm->fmap = (struct fid2name_map *) (pfn << PAGE_SHIFT);

	vm_add_fileid2name_map(vm, 0, "stdin");
	vm_add_fileid2name_map(vm, 1, "stdout");
	vm_add_fileid2name_map(vm, 2, "stderr");
}

void initialize_syscallmap(struct vm_info * vm)
{
	vm->nWaitingThreads = 0;

	const unsigned long pfn = alloc_host_pages (1, 1);
	vm->syscallmap = (struct tid2syscall_map *) (pfn << PAGE_SHIFT);
}

/******************************************************************************************/
/**************************** FILE ID 2 FILE NAME MAP *************************************/
/******************************************************************************************/

struct fid2name_map * __get_fid2name_entry(struct vm_info * vm, int index)
{
	return (struct fid2name_map *) ((void *) vm->fmap + index * sizeof(struct fid2name_map));
}

void vm_add_fileid2name_map(struct vm_info * vm, int id, char * name)
{
	struct fid2name_map * next = __get_fid2name_entry(vm, vm->nOpenFile);
	next->fid = id;
	memcpy(next->fname, name, VM_MAX_FILENAME);
	next->fname[VM_MAX_FILENAME - 1] = 0;

	vm->nOpenFile++;
}

void vm_remove_fileid2name_map(struct vm_info * vm, int id)
{
	int i;
	for (i = 0; i < vm->nOpenFile; i ++) {
		struct fid2name_map * entry = __get_fid2name_entry(vm, i);

		if (entry->fid == id) {
			int j;
			vm->nOpenFile--;
			for (j = i; j < vm->nOpenFile; j ++) {
				struct fid2name_map * cur = __get_fid2name_entry(vm, j);
				struct fid2name_map * next = __get_fid2name_entry(vm, j + 1);

				memcpy(cur, next, sizeof(struct fid2name_map));
			}
		}
	}
}

char * vm_get_fname_from_id(struct vm_info * vm, int id)
{
	int i;
	for (i = 0; i < vm->nOpenFile; i ++) {
		struct fid2name_map * entry = __get_fid2name_entry(vm, i);
		if (entry->fid == id) return entry->fname;
	}

	return NULL;
}

/******************************************************************************************/
/**************************** PROCESS TRACKING ********************************************/
/******************************************************************************************/

void initialize_ptracked_list(struct vm_info *vm)
{
	vm->btrackcurrent = 0;
	vm->nTrackedProcess = 0;

	const unsigned long pfn = alloc_host_pages (1, 1);
	vm->ptracked = (char *) (pfn << PAGE_SHIFT);

//	vm_add_tracked_process(vm, "loop"); //test
}

//index start from 0
char * __get_tracked_name_pointer(struct vm_info * vm, int index)
{
	return (char *) (vm->ptracked + VM_MAX_PNAME_LEN * index);
}

void vm_add_tracked_process(struct vm_info *vm, char * pname)
{
	if (vm_is_process_tracked(vm, pname)) return;

	char * nextSlot = __get_tracked_name_pointer(vm, vm->nTrackedProcess);
	strcpy(nextSlot, pname);

	vm->nTrackedProcess++;
}

void vm_remove_tracked_process(struct vm_info *vm, char * pname)
{
	int i;
	for (i = 0; i < vm->nTrackedProcess; i ++)
	{
		char * tracked_name = __get_tracked_name_pointer(vm, i);

		if (strcmp(tracked_name, pname) == 0)
		{
			vm->nTrackedProcess --;

			int j;
			for (j = i; j < vm->nTrackedProcess ; j ++)
			{
				char * cur = __get_tracked_name_pointer(vm, j);
				char * next = __get_tracked_name_pointer(vm, j + 1);
				strcpy(cur, next);
			}

			return;
		}
	}
}

int vm_is_process_tracked(struct vm_info *vm, char * pname)
{
	return 1;

	//TO DO: intercept specific process
	int i;
	for (i = 0; i < vm->nTrackedProcess; i ++)
	{
		char * tracked_name = __get_tracked_name_pointer(vm, i);
		if (strcmp(tracked_name, pname) == 0) return 1;
	}

	return 0;
}

/******************************************************************************************/
/*****************************  SYSCALL RETURN TRACKING ***********************************/
/******************************************************************************************/

struct tid2syscall_map * __get_syscallmap_entry(struct vm_info * vm, int index)
{
	return (struct tid2syscall_map *)
		((void *) vm->syscallmap + index * sizeof(struct tid2syscall_map));
}

void vm_add_waiting_thread(struct vm_info *vm, int tid, struct syscall_info * info)
{
	struct tid2syscall_map * newentry = __get_syscallmap_entry(vm, vm->nWaitingThreads);
	newentry->tid = tid;
	memcpy(&newentry->info, info, sizeof(struct syscall_info));

	vm->nWaitingThreads ++;
}

void vm_remove_waiting_thread(struct vm_info *vm, int tid) {
	int i;
	for (i = 0; i < vm->nWaitingThreads; i ++) {
		struct tid2syscall_map * entry = __get_syscallmap_entry(vm, i);

		if (entry->tid == tid) {
			int j;
			vm->nWaitingThreads--;
			for (j = i; j < vm->nWaitingThreads; j ++) {
				struct tid2syscall_map * cur = __get_syscallmap_entry(vm, j);
				struct tid2syscall_map * next = __get_syscallmap_entry(vm, j + 1);

				memcpy(cur, next, sizeof(struct tid2syscall_map));
			}
		}
	}
}

struct syscall_info * vm_get_waiting_syscall(struct vm_info *vm, int tid)
{
	int i;
	for (i = 0; i < vm->nWaitingThreads; i ++) {
		struct tid2syscall_map * entry = __get_syscallmap_entry(vm, i);
		if (entry->tid == tid) return &entry->info;
	}

	return NULL;
}

/******************************************************************************************/

void vm_create ( struct vm_info *vm, unsigned long vmm_pmem_start,
		unsigned long vmm_pmem_size, struct e820_map *e820)
{
	outf("\n++++++ Initializing guest VM info....\n");
	
	memset((char *)vm, 0, sizeof(struct vm_info));

	initialize_fid2name_map(vm);
	initialize_syscallmap(vm);
	initialize_ptracked_list(vm);

	vm->waitingRetSysCall = 0;
	
	/* Allocate new pages for physical memory of the guest OS.  */
	//const unsigned long vm_pmem_start = alloc_vm_pmem ( vm_pmem_size );
	//const unsigned long vm_pmem_start = 0x0; // guest is preallocated
	//vm->n_cr3 = create_4mb_nested_pagetable  (vmm_pmem_start, vmm_pmem_size, e820);
	vm->n_cr3 = create_2mb_nested_pagetable (vmm_pmem_start, vmm_pmem_size, e820);
}

// Identify CPU. Make sure it supports virtualzation feature.
// And then enable AMD SVM feature or Intel VMX feature according to the cpu type
void vm_init ( struct vm_info *vm )
{
	struct cpuinfo_x86 cpuinfo;

	early_identify_cpu (&cpuinfo);

	switch (cpuinfo.x86_vendor)
	{
	case X86_VENDOR_AMD:
		init_amd (vm, &cpuinfo);
		break;

	case X86_VENDOR_INTEL:
		init_intel (vm, &cpuinfo);
		break;

	case X86_VENDOR_UNKNOWN:
	default:
		fatal_failure ("Unknown CPU vendor\n");
		break;
	}
}

/**
 * to boot a linux vm, you'll need a copy of DSL that is checked out in a directory called dsl
 * I've uploaded it to http://www.cs.uiuc.edu/homes/nschear2/dsl-tvmm.tgz
 * enabled DSL in mkdisk.sh (set to 1)
 *
 * you'll need to do
 * ./configure
 * make
 * in the /tools/mkelfImage directory
 * then do (from mavmm directory)
 * ./tools/mkelfImage/objdir/sbin/mkelfImage --kernel dsl/boot/isolinux/linux24 --output dsl/boot/isolinux/elf-nabix
 *
 * next you'll need to update the disk you just made
 * ./updatedisk.sh test.hdd dsl/boot/isolinux/elf-nabix boot/isolinux
*/

/*****************************************************/

void vm_boot (struct vm_info *vm)
{
	//print_pg_table(vm->vmcb->n_cr3);
	int i=0;

	for(;;)
	{
//		vmcb_check_consistency ( vm->vmcb );
//		outf("\n=============================================\n");

		/* setup registers for boot  */
//		vm->vmcb->exitcode = 0;
//		outf("\n<<< %x >>> Guest state in VMCB:\n", i);
//		show_vmcb_state (vm); // Anh - print guest state in VMCB

		vm->vm_run (vm);

//		outf("\n<<< #%x >>> Guest state at VMEXIT:\n", i);

		vm->vm_exit (vm);

//		outf("\nTesting address translation function...\n");
//		unsigned long gphysic = glogic_2_gphysic(vm);
//		outf("Glogical %x:%x <=> Gphysical ", vm->vmcb->cs.sel, vm->vmcb->rip);
//		outf("%x\n", gphysic);
//		outf("=============================================\n");
		i++;

//		breakpoint("End of round...\n\n");
	}
}
