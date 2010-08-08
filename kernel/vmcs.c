/*
 * vmcs.c: VMCS management
 * Copyright (c) 2004, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 */

#include "types.h"
#include "bitops.h"
#include "string.h"
#include "serial.h"
#include "failure.h"
#include "page.h"
#include "msr.h"
#include "cpufeature.h"
#include "cpu.h"
#include "vmm.h"
#include "vmx.h"
#include "vmcs.h"
#include "alloc.h"
#include "intercept.h"

static int __read_mostly opt_vpid_enabled = 1;
static int __read_mostly opt_unrestricted_guest_enabled = 1;

/*
 * These two parameters are used to config the controls for Pause-Loop Exiting:
 * ple_gap:    upper bound on the amount of time between two successive
 *             executions of PAUSE in a loop.
 * ple_window: upper bound on the amount of time a guest is allowed to execute
 *             in a PAUSE loop.
 * Time is measured based on a counter that runs at the same rate as the TSC,
 * refer SDM volume 3b section 21.6.13 & 22.1.3.
 */
static unsigned int __read_mostly ple_gap = 41;
static unsigned int __read_mostly ple_window = 4096;

static u32 vmx_basic_msr_low __read_mostly;
static u32 vmx_basic_msr_high __read_mostly;

/* Dynamic (run-time adjusted) execution control flags. */
u32 vmx_pin_based_exec_control __read_mostly;
u32 vmx_pin_based_exec_default1 __read_mostly;
u32 vmx_pin_based_exec_default0 __read_mostly;

u32 vmx_cpu_based_exec_control __read_mostly;
u32 vmx_cpu_based_exec_default1 __read_mostly;
u32 vmx_cpu_based_exec_default0 __read_mostly;

u32 vmx_secondary_exec_control __read_mostly;
u32 vmx_secondary_exec_default1 __read_mostly;
u32 vmx_secondary_exec_default0 __read_mostly;

u32 vmx_vmexit_control __read_mostly;
u32 vmx_vmexit_default1 __read_mostly;
u32 vmx_vmexit_default0 __read_mostly;

u32 vmx_vmentry_control __read_mostly;
u32 vmx_vmentry_default1 __read_mostly;
u32 vmx_vmentry_default0 __read_mostly;

u64 vmx_ept_vpid_cap __read_mostly;
u8 cpu_has_vmx_ins_outs_instr_info __read_mostly;

static u32 vmcs_revision_id __read_mostly;

static void __init vmx_display_features(void)
{
    int printed = 0;

    outf("VMX: Supported advanced features:\n");

#define P(p,s) if ( p ) { outf(" - %s\n", s); printed = 1; }
    P(cpu_has_vmx_virtualize_apic_accesses, "APIC MMIO access virtualisation");
    P(cpu_has_vmx_tpr_shadow, "APIC TPR shadow");
    P(cpu_has_vmx_ept, "Extended Page Tables (EPT)");
    P(cpu_has_vmx_vpid, "Virtual-Processor Identifiers (VPID)");
    P(cpu_has_vmx_vnmi, "Virtual NMI");
    P(cpu_has_vmx_msr_bitmap, "MSR direct-access bitmap");
    P(cpu_has_vmx_unrestricted_guest, "Unrestricted Guest");
#undef P

    if ( !printed )
        outf(" - none\n");

    if ( cpu_has_vmx_ept_2mb )
        outf("EPT supports 2MB super page.\n");
}

/* VMX copabilities detection */
/* Intel IA-32 manual 3B 27.5.1 p. 222 */
void vmx_detect_capability(void)
{
    rdmsr(MSR_IA32_VMX_BASIC, &vmx_basic_msr_low, &vmx_basic_msr_high);

	/* save the revision_id */
    vmcs_revision_id = vmx_basic_msr_low;

	/* Determine the default1 and default0 for control msrs 
	 *
	 * Intel IA-32 manual 3B Appendix G.3 
	 *
	 * bit == 0 in msr high word ==> must be zero (default0, allowed1)
	 * bit == 1 in msr low word ==> must be one (default1, allowed0)
	 *
	 */

	if ( ! ( vmx_basic_msr_high & (1u<<23) ) )
	{
		// PIN BASED CONTROL
		rdmsr(MSR_IA32_VMX_PINBASED_CTLS, 
				&vmx_pin_based_exec_default1, &vmx_pin_based_exec_default0);

		// PROCESSOR(CPU) BASED CONTROL
		rdmsr(MSR_IA32_VMX_PROCBASED_CTLS,
				&vmx_cpu_based_exec_default1, &vmx_cpu_based_exec_default0);

		// VMEXIT CONTROL
		rdmsr(MSR_IA32_VMX_EXIT_CTLS,
				&vmx_vmexit_default1, &vmx_vmexit_default0);

		// VMENTRY CONTROL
		rdmsr(MSR_IA32_VMX_ENTRY_CTLS,
				&vmx_vmentry_default1, &vmx_vmentry_default0);
	}
	else /* if the 55 bit is 1 */
	{
		// PIN BASED CONTROL
		rdmsr(MSR_IA32_VMX_TRUE_PINBASED_CTLS, 
				&vmx_pin_based_exec_default1, &vmx_pin_based_exec_default0);

		// PROCESSOR(CPU) BASED CONTROL
		rdmsr(MSR_IA32_VMX_TRUE_PROCBASED_CTLS,
				&vmx_cpu_based_exec_default1, &vmx_cpu_based_exec_default0);

		// VMEXIT CONTROL
		rdmsr(MSR_IA32_VMX_TRUE_EXIT_CTLS,
				&vmx_vmexit_default1, &vmx_vmexit_default0);

		// VMENTRY CONTROL
		rdmsr(MSR_IA32_VMX_TRUE_ENTRY_CTLS,
				&vmx_vmentry_default1, &vmx_vmentry_default0);
	}

    // detect EPT and VPID capability
	if ( vmx_cpu_based_exec_default1 & CPU_BASED_ACTIVATE_SECONDARY_CONTROLS )
    {
        rdmsr(MSR_IA32_VMX_PROCBASED_CTLS2, 
				&vmx_secondary_exec_default1, &vmx_secondary_exec_default0);

		/* The IA32_VMX_EPT_VPID_CAP MSR exists only when EPT or VPID available */
		if ( vmx_secondary_exec_default1 & (SECONDARY_EXEC_ENABLE_EPT |
                                        SECONDARY_EXEC_ENABLE_VPID) )
		{
			rdmsrl(MSR_IA32_VMX_EPT_VPID_CAP, &vmx_ept_vpid_cap);
		}
	}

    if ( !vmx_pin_based_exec_control )
    {
        /* First time through. */
        vmcs_revision_id = vmx_basic_msr_low;
        vmx_pin_based_exec_control = vmx_pin_based_exec_default1;
        vmx_cpu_based_exec_control = vmx_cpu_based_exec_default1;
        vmx_secondary_exec_control = vmx_secondary_exec_default1;
        vmx_vmexit_control         = vmx_vmexit_default1;
        vmx_vmentry_control        = vmx_vmentry_default1;
        cpu_has_vmx_ins_outs_instr_info = !!(vmx_basic_msr_high & (1U<<22));
        //vmx_display_features();
    }
}

static struct vmcs *alloc_vmcs ( void )
{
    struct vmcs *vmcs;

	const unsigned long pfn = alloc_host_pages (1, 1);
	//outf("Free page for vmcb: %x\n", pfn);
	vmcs = (struct vmcs *) (pfn << PAGE_SHIFT);
	memset (( char *) vmcs, 0, PAGE_SIZE);

	return vmcs;
}

struct vmcs* create_vmcs ( void )
{
	/* verify settings */
	
    /* IA-32 SDM Vol 3B: VMCS size is never greater than 4kB. */
    if ( (vmx_basic_msr_high & 0x1fff) > PAGE_SIZE)
	{
		outf("VMCS size larger than 4K\n");
		return NULL;
	}

    /* IA-32 SDM Vol 3B: 64-bit CPUs always have VMX_BASIC_MSR[48]==0. */
    if ( vmx_basic_msr_high & (1u<<16) )
	{
		outf("VMX_BASIC_MSR[48] = 1\n");
		return NULL;
	}

    /* Require Write-Back (WB) memory type for VMCS accesses. */
    if ( ( (vmx_basic_msr_high >> 18) & 15) != 6 )
	{
		outf("Write-back memory required for VMCS\n");
		return NULL;
	}
	
	/* Alloc a page for vmcs */
	struct vmcs* vmcs = alloc_vmcs();
	if ( vmcs != NULL ) {
		vmcs->revision_id = vmcs_revision_id;
	}
	
	return vmcs;
}

static void vmcs_init_host_env()
{
	unsigned long cr0, cr4;
	struct {
		u16 limit;
		u64 base;
	} __attribute__ ((packed)) xdt;

    /* Host data selectors. */
    __vmwrite(HOST_SS_SELECTOR, VMM_DS64);
    __vmwrite(HOST_DS_SELECTOR, VMM_DS64);
    __vmwrite(HOST_ES_SELECTOR, VMM_DS64);
    __vmwrite(HOST_FS_SELECTOR, VMM_DS64);
    __vmwrite(HOST_GS_SELECTOR, VMM_DS64);
    __vmwrite(HOST_FS_BASE, 0);
    __vmwrite(HOST_GS_BASE, 0);

    /* Host control registers. */
    cr0 = read_cr0() | X86_CR0_TS;
    __vmwrite(HOST_CR0, cr0);

	cr4 = read_cr4() | X86_CR4_OSXSAVE;
    __vmwrite(HOST_CR4, cr4);

    /* Host CS:RIP. */
    __vmwrite(HOST_CS_SELECTOR, VMM_CS64);
    __vmwrite(HOST_RIP, (unsigned long)vmx_asm_vmexit_handler);

    /* Host SYSENTER CS:RIP. */
    __vmwrite(HOST_SYSENTER_CS, 0);
    __vmwrite(HOST_SYSENTER_EIP, 0);
    __vmwrite(HOST_SYSENTER_ESP, 0);

	/* GDT */
	__asm__ __volatile__ ("sgdt (%0) \n" :: "a"(&xdt) : "memory");
	__vmwrite(HOST_GDTR_BASE, xdt.base);

	/* IDT */
	__asm__ __volatile__ ("sidt (%0) \n" :: "a"(&xdt) : "memory");
    __vmwrite(HOST_IDTR_BASE, xdt.base);

	/* TR */
    __vmwrite(HOST_TR_SELECTOR, VMM_DS64);
    __vmwrite(HOST_TR_BASE, 0);

}

void vmx_set_control_params (struct vm_info *vm)
{
	/* Initialize pin based control */
	__vmwrite(PIN_BASED_VM_EXEC_CONTROL, vmx_pin_based_exec_control);

	/* Initialize cpu based control */
	vmx_cpu_based_exec_control |= CPU_BASED_ACTIVATE_SECONDARY_CONTROLS;

	// IO bitmap
	vmx_cpu_based_exec_control |= CPU_BASED_ACTIVATE_IO_BITMAP;

	vm->io_intercept_table = create_intercept_table(8 << 10); // 8KB, 4KB each
	__vmwrite(IO_BITMAP_A, vm->io_intercept_table);
	__vmwrite(IO_BITMAP_B, vm->io_intercept_table + PAGE_SIZE);

	// MSR bitmap
	vmx_cpu_based_exec_control |= CPU_BASED_ACTIVATE_MSR_BITMAP;

	vm->msr_intercept_table = create_intercept_table(4 << 10); // 4KB
	__vmwrite(MSR_BITMAP, vm->msr_intercept_table);

	__vmwrite(CPU_BASED_VM_EXEC_CONTROL, vmx_cpu_based_exec_control);

	/* Enable Extended Page Table (nested paging) */
	vmx_secondary_exec_control |= SECONDARY_EXEC_ENABLE_EPT;

	// setup eptp
	vmx_ept_control.ept_mt = 6; // Memory type WriteBack
	vmx_ept_control.ept_wl = 3; // Page-walk length-1 
	vmx_ept_control.rsvd = 0; // reserved
	vmx_ept_control.asr = vm->n_cr3; // nested cr3

	__vmwrite(EPT_POINTER, vmx_ept_control.eptp);
	
	/* Enable Virtual-Processor Identification (asid) */
	vmx_secondary_exec_control |= SECONDARY_EXEC_ENABLE_VPID;

	__vmwrite(VIRTUAL_PROCESSOR_ID, 1);

	/* Initialize vm exit controls */
	vmx_vmexit_control |= (VM_EXIT_IA32E_MODE | VM_EXIT_ACK_INTR_ON_EXIT);
	vmx_vmexit_control |= (VM_EXIT_SAVE_GUEST_PAT | VM_EXIT_LOAD_HOST_PAT);

	__vmwrite(VM_EXIT_CONTROLS, vmx_vmexit_control);

	/* Initialize vm entry controls */
	vmx_vmentry_control |= VM_ENTRY_LOAD_GUEST_PAT;

	__vmwrite(VM_ENTRY_CONTROLS, vmx_vmentry_control);

	/* Initialize host save area */
	vmcs_init_host_env();
}

struct xgt_desc {
    unsigned short size;
    unsigned long address __attribute__((packed));
};

void vmx_save_host_state(struct vm_info *vm)
{
	unsigned long rsp;

    /*
     * Skip end of cpu_user_regs when entering the hypervisor because the
     * CPU does not save context onto the stack. SS,RSP,CS,RIP,RFLAGS,etc
     * all get saved into the VMCS instead.
     */
	__asm__ __volatile__ ("movq %%rsp, %0 \n\t" : "=r"(rsp));
    __vmwrite(HOST_RSP, rsp);
}

void vmx_disable_intercept_for_msr(struct vm_info *vm, u32 msr)
{
    unsigned long *msr_bitmap = (unsigned long *)vm->msr_intercept_table;

    /* VMX MSR bitmap supported? */
    if ( msr_bitmap == NULL )
        return;

    /*
     * See Intel PRM Vol. 3, 20.6.9 (MSR-Bitmap Address). Early manuals
     * have the write-low and read-high bitmap offsets the wrong way round.
     * We can control MSRs 0x00000000-0x00001fff and 0xc0000000-0xc0001fff.
     */
    if ( msr <= 0x1fff )
    {
        clear_bit(msr, msr_bitmap + 0x000/BYTES_PER_LONG); /* read-low */
        clear_bit(msr, msr_bitmap + 0x800/BYTES_PER_LONG); /* write-low */
    }
    else if ( (msr >= 0xc0000000) && (msr <= 0xc0001fff) )
    {
        msr &= 0x1fff;
        clear_bit(msr, msr_bitmap + 0x400/BYTES_PER_LONG); /* read-high */
        clear_bit(msr, msr_bitmap + 0xc00/BYTES_PER_LONG); /* write-high */
    }
}

void vmx_set_vm_to_mbr_start_state(struct vm_info *vm)
{

    /* MSR intercepts. */
    __vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
    __vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
    __vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);

    __vmwrite(VM_ENTRY_INTR_INFO, 0);

    __vmwrite(CR0_GUEST_HOST_MASK, ~0UL);
    __vmwrite(CR4_GUEST_HOST_MASK, ~0UL);

    __vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
    __vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

    __vmwrite(CR3_TARGET_COUNT, 0);

    __vmwrite(GUEST_ACTIVITY_STATE, 0);

    /* Guest segment bases. */
    __vmwrite(GUEST_ES_BASE, 0);
    __vmwrite(GUEST_SS_BASE, 0);
    __vmwrite(GUEST_DS_BASE, 00000400);
    __vmwrite(GUEST_FS_BASE, 0xE7170);
    __vmwrite(GUEST_GS_BASE, 0xF0000);
    __vmwrite(GUEST_CS_BASE, 0);

    /* Guest segment limits. */
    __vmwrite(GUEST_ES_LIMIT, ~0u);
    __vmwrite(GUEST_SS_LIMIT, ~0u);
    __vmwrite(GUEST_DS_LIMIT, ~0u);
    __vmwrite(GUEST_FS_LIMIT, ~0u);
    __vmwrite(GUEST_GS_LIMIT, ~0u);
    __vmwrite(GUEST_CS_LIMIT, ~0u);

    /* Guest segment AR bytes. */
    __vmwrite(GUEST_ES_AR_BYTES, 0x93);
    __vmwrite(GUEST_SS_AR_BYTES, 0x193);
    __vmwrite(GUEST_DS_AR_BYTES, 0x93);
    __vmwrite(GUEST_FS_AR_BYTES, 0x93);
    __vmwrite(GUEST_GS_AR_BYTES, 0x93);
    __vmwrite(GUEST_CS_AR_BYTES, 0x19b);

	/* Guest segment selector. */
	__vmwrite(GUEST_ES_SELECTOR, 0);
	__vmwrite(GUEST_SS_SELECTOR, 0);
	__vmwrite(GUEST_DS_SELECTOR, 0x0040);
	__vmwrite(GUEST_FS_SELECTOR, 0xE717);
	__vmwrite(GUEST_GS_SELECTOR, 0xF000);
	__vmwrite(GUEST_CS_SELECTOR, 0);

    /* Guest IDT. */
    __vmwrite(GUEST_IDTR_BASE, 0);
    __vmwrite(GUEST_IDTR_LIMIT, 0);

    /* Guest GDT. */
    __vmwrite(GUEST_GDTR_BASE, 0);
    __vmwrite(GUEST_GDTR_LIMIT, 0);

    /* Guest LDT. */
    __vmwrite(GUEST_LDTR_AR_BYTES, 0x0082); /* LDT */
    __vmwrite(GUEST_LDTR_SELECTOR, 0);
    __vmwrite(GUEST_LDTR_BASE, 0);
    __vmwrite(GUEST_LDTR_LIMIT, 0);

    /* Guest TSS. */
    __vmwrite(GUEST_TR_AR_BYTES, 0x008b); /* 32-bit TSS (busy) */
    __vmwrite(GUEST_TR_BASE, 0);
    __vmwrite(GUEST_TR_LIMIT, 0xff);

    __vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
    __vmwrite(GUEST_DR7, 0);
    __vmwrite(VMCS_LINK_POINTER, ~0UL);

    __vmwrite(EXCEPTION_BITMAP, 0);

    /* Control registers */
	__vmwrite(GUEST_CR0, (X86_CR0_PE | X86_CR0_ET));
	__vmwrite(GUEST_CR3, 0);
	__vmwrite(GUEST_CR4, 0);

	/* G_PAT */
    u64 host_pat, guest_pat;

    rdmsrl(MSR_IA32_CR_PAT, &host_pat);
    guest_pat = MSR_IA32_CR_PAT_RESET;

    __vmwrite(HOST_PAT, host_pat);
    __vmwrite(GUEST_PAT, guest_pat);

	/* Initial state */
	__vmwrite(GUEST_RSP, 0x3E2);
	__vmwrite(GUEST_RFLAGS, 0x2206);
	__vmwrite(GUEST_RIP, 0x7C00);
}

int vmx_read_guest_msr(struct vm_info *vm, u32 msr, u64 *val)
{
    unsigned int i, msr_count = vm->msr_count;
    const struct vmx_msr_entry *msr_area = vm->msr_area;

    for ( i = 0; i < msr_count; i++ )
    {
        if ( msr_area[i].index == msr )
        {
            *val = msr_area[i].data;
            return 0;
        }
    }

    return -1;
}

int vmx_write_guest_msr(struct vm_info *vm, u32 msr, u64 val)
{
    unsigned int i, msr_count = vm->msr_count;
    struct vmx_msr_entry *msr_area = vm->msr_area;

    for ( i = 0; i < msr_count; i++ )
    {
        if ( msr_area[i].index == msr )
        {
            msr_area[i].data = val;
            return 0;
        }
    }

    return -1;
}

int vmx_add_guest_msr(struct vm_info *vm, u32 msr)
{
    unsigned int i, msr_count = vm->msr_count;
    struct vmx_msr_entry *msr_area = vm->msr_area;

    if ( msr_area == NULL )
    {
        if ( (msr_area = (struct vmx_msr_entry *)alloc_host_pages(1, 1)) == NULL )
            return -2;
        vm->msr_area = msr_area;
        __vmwrite(VM_EXIT_MSR_STORE_ADDR, (u64)msr_area);
        __vmwrite(VM_ENTRY_MSR_LOAD_ADDR, (u64)msr_area);
    }

    for ( i = 0; i < msr_count; i++ )
        if ( msr_area[i].index == msr )
            return 0;

    if ( msr_count == (PAGE_SIZE / sizeof(struct vmx_msr_entry)) )
        return -1;

    msr_area[msr_count].index = msr;
    msr_area[msr_count].mbz   = 0;
    msr_area[msr_count].data  = 0;
    vm->msr_count = ++msr_count;
    __vmwrite(VM_EXIT_MSR_STORE_COUNT, msr_count);
    __vmwrite(VM_ENTRY_MSR_LOAD_COUNT, msr_count);

    return 0;
}

int vmx_add_host_load_msr(struct vm_info *vm, u32 msr)
{
    unsigned int i, msr_count = vm->host_msr_count;
    struct vmx_msr_entry *msr_area = vm->host_msr_area;

    if ( msr_area == NULL )
    {
        if ( (msr_area = (struct vmx_msr_entry *)alloc_host_pages(1, 1)) == NULL )
            return -2;
        vm->host_msr_area = msr_area;
        __vmwrite(VM_EXIT_MSR_LOAD_ADDR, (u64)msr_area);
    }

    for ( i = 0; i < msr_count; i++ )
        if ( msr_area[i].index == msr )
            return 0;

    if ( msr_count == (PAGE_SIZE / sizeof(struct vmx_msr_entry)) )
        return -1;

    msr_area[msr_count].index = msr;
    msr_area[msr_count].mbz   = 0;
    rdmsrl(msr, &msr_area[msr_count].data);
    vm->host_msr_count = ++msr_count;
    __vmwrite(VM_EXIT_MSR_LOAD_COUNT, msr_count);

    return 0;
}

void vm_launch_fail(void)
{
    unsigned long error = __vmread(VM_INSTRUCTION_ERROR);
    outf("<vm_launch_fail> error code %lx\n", error);
}

void vm_resume_fail(void)
{
    unsigned long error = __vmread(VM_INSTRUCTION_ERROR);
    outf("<vm_resume_fail> error code %lx\n", error);
}

void vmx_do_resume(struct vm_info *vm)
{
    //reset_stack_and_jump(vmx_asm_do_vmentry);
}

static unsigned long vmr(unsigned long field)
{
    int rc;
    unsigned long val;
    val = __vmread_safe(field, &rc);
    return rc ? 0 : val;
}

static void vmx_dump_sel(char *name, u32 selector)
{
    u32 sel, attr, limit;
    u64 base;
    sel = vmr(selector);
    attr = vmr(selector + (GUEST_ES_AR_BYTES - GUEST_ES_SELECTOR));
    limit = vmr(selector + (GUEST_ES_LIMIT - GUEST_ES_SELECTOR));
    base = vmr(selector + (GUEST_ES_BASE - GUEST_ES_SELECTOR));
    outf("%s: sel=0x%04x, attr=0x%05x, limit=0x%08x, base=0x%016x\n",
           name, sel, attr, limit, base);
}

static void vmx_dump_sel2(char *name, u32 lim)
{
    u32 limit;
    u64 base;
    limit = vmr(lim);
    base = vmr(lim + (GUEST_GDTR_BASE - GUEST_GDTR_LIMIT));
    outf("%s:                           limit=0x%08x, base=0x%016x\n",
           name, limit, base);
}

void vmcs_dump(struct vm_info *vm)
{
    unsigned long long x;

    outf("*** Guest State ***\n");
    outf("CR0: actual=0x%016llx, shadow=0x%016llx, gh_mask=%016llx\n",
           (unsigned long long)vmr(GUEST_CR0),
           (unsigned long long)vmr(CR0_READ_SHADOW), 
           (unsigned long long)vmr(CR0_GUEST_HOST_MASK));
    outf("CR4: actual=0x%016llx, shadow=0x%016llx, gh_mask=%016llx\n",
           (unsigned long long)vmr(GUEST_CR4),
           (unsigned long long)vmr(CR4_READ_SHADOW), 
           (unsigned long long)vmr(CR4_GUEST_HOST_MASK));
    outf("CR3: actual=0x%016llx, target_count=%d\n",
           (unsigned long long)vmr(GUEST_CR3),
           (int)vmr(CR3_TARGET_COUNT));
    outf("     target0=%016llx, target1=%016llx\n",
           (unsigned long long)vmr(CR3_TARGET_VALUE0),
           (unsigned long long)vmr(CR3_TARGET_VALUE1));
    outf("     target2=%016llx, target3=%016llx\n",
           (unsigned long long)vmr(CR3_TARGET_VALUE2),
           (unsigned long long)vmr(CR3_TARGET_VALUE3));
    outf("RSP = 0x%016llx, RIP = 0x%016llx\n", 
           (unsigned long long)vmr(GUEST_RSP),
           (unsigned long long)vmr(GUEST_RIP));
    outf("RFLAGS=0x%016llx DR7 = 0x%016llx\n", 
           (unsigned long long)vmr(GUEST_RFLAGS),
           (unsigned long long)vmr(GUEST_DR7));
    outf("Sysenter RSP=%016llx CS:RIP=%04x:%016llx\n",
           (unsigned long long)vmr(GUEST_SYSENTER_ESP),
           (int)vmr(GUEST_SYSENTER_CS),
           (unsigned long long)vmr(GUEST_SYSENTER_EIP));
    vmx_dump_sel("CS", GUEST_CS_SELECTOR);
    vmx_dump_sel("DS", GUEST_DS_SELECTOR);
    vmx_dump_sel("SS", GUEST_SS_SELECTOR);
    vmx_dump_sel("ES", GUEST_ES_SELECTOR);
    vmx_dump_sel("FS", GUEST_FS_SELECTOR);
    vmx_dump_sel("GS", GUEST_GS_SELECTOR);
    vmx_dump_sel2("GDTR", GUEST_GDTR_LIMIT);
    vmx_dump_sel("LDTR", GUEST_LDTR_SELECTOR);
    vmx_dump_sel2("IDTR", GUEST_IDTR_LIMIT);
    vmx_dump_sel("TR", GUEST_TR_SELECTOR);
    outf("Guest PAT = 0x%08x%08x\n",
           (u32)vmr(GUEST_PAT_HIGH), (u32)vmr(GUEST_PAT));
    x  = (unsigned long long)vmr(TSC_OFFSET_HIGH) << 32;
    x |= (u32)vmr(TSC_OFFSET);
    outf("TSC Offset = %016llx\n", x);
    x  = (unsigned long long)vmr(GUEST_IA32_DEBUGCTL_HIGH) << 32;
    x |= (u32)vmr(GUEST_IA32_DEBUGCTL);
    outf("DebugCtl=%016llx DebugExceptions=%016llx\n", x,
           (unsigned long long)vmr(GUEST_PENDING_DBG_EXCEPTIONS));
    outf("Interruptibility=%04x ActivityState=%04x\n",
           (int)vmr(GUEST_INTERRUPTIBILITY_INFO),
           (int)vmr(GUEST_ACTIVITY_STATE));

    outf("*** Host State ***\n");
    outf("RSP = 0x%016llx  RIP = 0x%016llx\n", 
           (unsigned long long)vmr(HOST_RSP),
           (unsigned long long)vmr(HOST_RIP));
    outf("CS=%04x DS=%04x ES=%04x FS=%04x GS=%04x SS=%04x TR=%04x\n",
           (u16)vmr(HOST_CS_SELECTOR),
           (u16)vmr(HOST_DS_SELECTOR),
           (u16)vmr(HOST_ES_SELECTOR),
           (u16)vmr(HOST_FS_SELECTOR),
           (u16)vmr(HOST_GS_SELECTOR),
           (u16)vmr(HOST_SS_SELECTOR),
           (u16)vmr(HOST_TR_SELECTOR));
    outf("FSBase=%016llx GSBase=%016llx TRBase=%016llx\n",
           (unsigned long long)vmr(HOST_FS_BASE),
           (unsigned long long)vmr(HOST_GS_BASE),
           (unsigned long long)vmr(HOST_TR_BASE));
    outf("GDTBase=%016llx IDTBase=%016llx\n",
           (unsigned long long)vmr(HOST_GDTR_BASE),
           (unsigned long long)vmr(HOST_IDTR_BASE));
    outf("CR0=%016llx CR3=%016llx CR4=%016llx\n",
           (unsigned long long)vmr(HOST_CR0),
           (unsigned long long)vmr(HOST_CR3),
           (unsigned long long)vmr(HOST_CR4));
    outf("Sysenter RSP=%016llx CS:RIP=%04x:%016llx\n",
           (unsigned long long)vmr(HOST_SYSENTER_ESP),
           (int)vmr(HOST_SYSENTER_CS),
           (unsigned long long)vmr(HOST_SYSENTER_EIP));
    outf("Host PAT = 0x%08x%08x\n",
           (u32)vmr(HOST_PAT_HIGH), (u32)vmr(HOST_PAT));

    outf("*** Control State ***\n");
    outf("PinBased=%08x CPUBased=%08x SecondaryExec=%08x\n",
           (u32)vmr(PIN_BASED_VM_EXEC_CONTROL),
           (u32)vmr(CPU_BASED_VM_EXEC_CONTROL),
           (u32)vmr(SECONDARY_VM_EXEC_CONTROL));
    outf("EntryControls=%08x ExitControls=%08x\n",
           (u32)vmr(VM_ENTRY_CONTROLS),
           (u32)vmr(VM_EXIT_CONTROLS));
    outf("ExceptionBitmap=%08x\n",
           (u32)vmr(EXCEPTION_BITMAP));
    outf("VMEntry: intr_info=%08x errcode=%08x ilen=%08x\n",
           (u32)vmr(VM_ENTRY_INTR_INFO),
           (u32)vmr(VM_ENTRY_EXCEPTION_ERROR_CODE),
           (u32)vmr(VM_ENTRY_INSTRUCTION_LEN));
    outf("VMExit: intr_info=%08x errcode=%08x ilen=%08x\n",
           (u32)vmr(VM_EXIT_INTR_INFO),
           (u32)vmr(VM_EXIT_INTR_ERROR_CODE),
           (u32)vmr(VM_ENTRY_INSTRUCTION_LEN));
    outf("        reason=%08x qualification=%08x\n",
           (u32)vmr(VM_EXIT_REASON),
           (u32)vmr(EXIT_QUALIFICATION));
    outf("IDTVectoring: info=%08x errcode=%08x\n",
           (u32)vmr(IDT_VECTORING_INFO),
           (u32)vmr(IDT_VECTORING_ERROR_CODE));
    outf("TPR Threshold = 0x%02x\n",
           (u32)vmr(TPR_THRESHOLD));
    outf("EPT pointer = 0x%08x%08x\n",
           (u32)vmr(EPT_POINTER_HIGH), (u32)vmr(EPT_POINTER));
    outf("Virtual processor ID = 0x%04x\n",
           (u32)vmr(VIRTUAL_PROCESSOR_ID));

}

