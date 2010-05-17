/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "string.h"
#include "failure.h"
#include "page.h"
#include "msr.h"
#include "vm.h"
#include "svm.h"
#include "alloc.h"
#include "vmexit.h"
#include "vmm.h"
#include "intercept.h"
#include "serial.h"
#include "types.h"
#include "user.h"
#include "system.h"

static struct vmcb * alloc_vmcb ( void )
{
	struct vmcb *vmcb;

	const unsigned long pfn = alloc_host_pages (1, 1);
	//outf("Free page for vmcb: %x\n", pfn);
	vmcb = (struct vmcb *) (pfn << PAGE_SHIFT);
	memset (( char *) vmcb, 0, sizeof (struct vmcb));

	return vmcb;
}

static unsigned long create_intercept_table ( unsigned long size )
{
	const unsigned long pfn = alloc_host_pages ( size >> PAGE_SHIFT, 1 );
	void *p = ( void * ) ( pfn << PAGE_SHIFT );

	/* vol. 2, p. 445 */
	//memset ( p, 0xff, size );
	memset ( p, 0x00, size );

	return pfn << PAGE_SHIFT;
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

/********************************************************************************************/
/******************************* NESTED PAGING PROTECTION ***********************************/
/********************************************************************************************/

void set_4mb_pagetable_attr (u32 cr3, u32 linearaddr, u16 attr)
{
	// identity map
	mmap_4mb (cr3, linearaddr, linearaddr, attr);
}

u16 get_4mb_pagetable_attr (u32 cr3, u32 linearaddr)
{
	int index = linearaddr >> PAGE_SHIFT_4MB;
	struct pd4M_entry * entry = (struct pd4M_entry *) (cr3 + index * sizeof(struct pd4M_entry));

	return entry->flags;
}

void __vm_protect_all_nonPAE_page(u32 cr3)
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

void __vm_unprotect_nonPAE_page(u32 cr3)
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

/********************************************************************************************/
/********************************* INITIALIZE VM STATE *************************************/
/********************************************************************************************/

//Anh - Test, to set initial state of VM to state of machine right after power up
void set_vm_to_powerup_state(struct vmcb * vmcb)
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

void set_vm_to_mbr_start_state(struct vmcb* vmcb)
{
	// Prepare to load GRUB for the second time
	// Basically copy the state when GRUB is first started
	// Note: some other states will be set in svm_asm.S, at load_guest_states:
	// ebx, ecx, edx, esi, edi, ebp

	memset(vmcb, 0, sizeof(vmcb));

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
	vmcb->g_pat = 0x7040600070406UL;

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
	outf("\n++++++ Creating guest VM....\n");

	// Allocate a new page inside host memory for storing VMCB.
	struct vmcb *vmcb;
	vmcb = alloc_vmcb();
	vm->vmcb = vmcb;
	outf("VMCB location: %x\n", vmcb);

	/* Allocate new pages for physical memory of the guest OS.  */
	//const unsigned long vm_pmem_start = alloc_vm_pmem ( vm_pmem_size );
	//const unsigned long vm_pmem_start = 0x0; // guest is preallocated

	/* Set Host-level CR3 to use for nested paging.  */
	vm->n_cr3 = create_4mb_nested_pagetable  (vmm_pmem_start, vmm_pmem_size, e820);
	vmcb->n_cr3 = vm->n_cr3;

	//Set control params for this VM
	//such as whether to enable nested paging, what to intercept...
	set_control_params (vm->vmcb);

	// Set VM initial state
	// Guest VM start at MBR code, which is GRUB stage 1
	// vmcb->rip = 0x7c00; address of loaded MBR
	set_vm_to_mbr_start_state(vmcb);

	initialize_fid2name_map(vm);
	initialize_syscallmap(vm);
	initialize_ptracked_list(vm);

	vm->waitingRetSysCall = 0;
}

static void switch_to_guest_os ( struct vm_info *vm )
{
//	outf(">>>>>>>>>>> GOING INTO THE MATRIX!!!!!\n");
	//u64 p_vmcb = vm->vmcb;

	/* Set the pointer to VMCB to %rax (vol. 2, p. 440) */
	__asm__("push %%eax; mov %0, %%eax" :: "r" (vm->vmcb));

	svm_launch ();

	__asm__("pop %eax");
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

		switch_to_guest_os (vm);

//		outf("\n<<< #%x >>> Guest state at VMEXIT:\n", i);

		handle_vmexit (vm);

//		outf("\nTesting address translation function...\n");
//		unsigned long gphysic = glogic_2_gphysic(vm);
//		outf("Glogical %x:%x <=> Gphysical ", vm->vmcb->cs.sel, vm->vmcb->rip);
//		outf("%x\n", gphysic);
//		outf("=============================================\n");
		i++;

//		breakpoint("End of round...\n\n");
	}
}
