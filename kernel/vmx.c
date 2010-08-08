/*
 * vmx.c: handling VMX architecture-related operations
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
#include "vmx.h"
#include "alloc.h"

/* VMM Setup */
/* Intel IA-32 Manual 3B 27.5 p. 221 */
static void enable_vmx (struct cpuinfo_x86 *cpuinfo)
{
	u32 eax, edx;
    int bios_locked;
    u64 cr0, vmx_cr0_fixed0, vmx_cr0_fixed1; 

	/* Xen does not fill x86_capability words except 0. */
    cpuinfo->x86_capability[5] = cpuid_ecx(1);

	/* Detect VMX support */
	if ( ! (test_bit(X86_FEATURE_VMX, &cpuinfo->x86_capability) ) ) {
		fatal_failure ("No VMX feature!\n");
		return;
	}
	
	/* Determine the VMX capabilities */
    vmx_detect_capability();
    
    /* EPT and VPID support is required */
    if ( ! cpu_has_vmx_ept) {
		fatal_failure ("No EPT support!\n");
		return;
	}
	
	/*if ( ! cpu_has_vmx_vpid ) {
		fatal_failure ("No VPID support!\n");
		return;
	}*/
	
	/* Enable VMX operation */
	set_in_cr4(X86_CR4_VMXE);
	
    /* 
     * Ensure the current processor operating mode meets 
     * the requred CRO fixed bits in VMX operation. 
     */
    cr0 = read_cr0();
    rdmsrl(MSR_IA32_VMX_CR0_FIXED0, &vmx_cr0_fixed0);
    rdmsrl(MSR_IA32_VMX_CR0_FIXED1, &vmx_cr0_fixed1);
    if ( (~cr0 & vmx_cr0_fixed0) || (cr0 & ~vmx_cr0_fixed1) )
    {
        fatal_failure("Some settings of host CR0 are not allowed in VMX operation.\n");
        return;
    }

    rdmsr(IA32_FEATURE_CONTROL_MSR, &eax, &edx);

	/*
	 * Ensure that the IA32_FEATURE_CONTROL MSR has been
	 * properly programmed.
	 */
    bios_locked = !!(eax & IA32_FEATURE_CONTROL_MSR_LOCK);
    if ( bios_locked )
    {
        if ( !(eax & /*(tboot_in_measured_env()
                      ? IA32_FEATURE_CONTROL_MSR_ENABLE_VMXON_INSIDE_SMX
                      : IA32_FEATURE_CONTROL_MSR_ENABLE_VMXON_OUTSIDE_SMX)*/
					IA32_FEATURE_CONTROL_MSR_ENABLE_VMXON_OUTSIDE_SMX) )
        {
            fatal_failure("VMX disabled by BIOS.\n");
            return;
        }
    }
    /* lock VMXON, prevent guest from turnning on this feature */
	/*else
    {
        eax  = IA32_FEATURE_CONTROL_MSR_LOCK;
        eax |= IA32_FEATURE_CONTROL_MSR_ENABLE_VMXON_OUTSIDE_SMX;
        if ( test_bit(X86_FEATURE_SMX, &cpuinfo->x86_capability) )
            eax |= IA32_FEATURE_CONTROL_MSR_ENABLE_VMXON_INSIDE_SMX;
        wrmsr(IA32_FEATURE_CONTROL_MSR, eax, 0);
    }*/
    
}

void __init init_intel (struct vm_info *vm, struct cpuinfo_x86 *cpuinfo)
{
	/* Enable VMX */
	enable_vmx(cpuinfo);
	
	/* Create a VMXON region */
    struct vmcs *vmcs;
    vmcs = create_vmcs();
    if ( vmcs == NULL ) {
    	fatal_failure ("Failed to create VMXON region.\n");
    	return;
    }
    vm->vmcs = vmcs;
    outf("VMCS location: %x\n", vmcs);
    
    /* VMXON */
    if ( __vmxon( (u64)vm->vmcs ) != 0 ) {
    	fatal_failure("VMXON failure\n");
        return;  
    }

	/* VMCLEAR */
	__vmpclear( (u64)vm->vmcs );

	/* VMPTRLD */
	__vmptrld( (u64)vm->vmcs );
    
    vmx_set_control_params ( vm );
    
    vmx_set_vm_to_mbr_start_state ( vm );
}
