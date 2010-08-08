/*
 * vmx.h: VMX Architecture related definitions
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
 *
 */
#ifndef __VMX_H__
#define __VMX_H__

#include "types.h"
#include "cpu.h"
#include "vmcs.h"

typedef union {
    struct {
        u64 r       :   1,
        w           :   1,
        x           :   1,
        emt         :   3, /* EPT Memory type */
        ipat        :   1, /* Ignore PAT memory type */
        sp          :   1, /* Is this a superpage? */
        avail1      :   4,
        mfn         :   40,
        avail2      :   12;
    };
    u64 epte;
} ept_entry_t;

#define EPT_TABLE_ORDER         9
#define EPTE_SUPER_PAGE_MASK    0x80
#define EPTE_MFN_MASK           0xffffffffff000ULL
#define EPTE_AVAIL1_MASK        0xF00
#define EPTE_EMT_MASK           0x38
#define EPTE_IGMT_MASK          0x40
#define EPTE_AVAIL1_SHIFT       8
#define EPTE_EMT_SHIFT          3
#define EPTE_IGMT_SHIFT         6

/*
 * Exit Reasons
 */
#define VMX_EXIT_REASONS_FAILED_VMENTRY 0x80000000

#define EXIT_REASON_EXCEPTION_NMI       0
#define EXIT_REASON_EXTERNAL_INTERRUPT  1
#define EXIT_REASON_TRIPLE_FAULT        2
#define EXIT_REASON_INIT                3
#define EXIT_REASON_SIPI                4
#define EXIT_REASON_IO_SMI              5
#define EXIT_REASON_OTHER_SMI           6
#define EXIT_REASON_PENDING_VIRT_INTR   7
#define EXIT_REASON_PENDING_VIRT_NMI    8
#define EXIT_REASON_TASK_SWITCH         9
#define EXIT_REASON_CPUID               10
#define EXIT_REASON_HLT                 12
#define EXIT_REASON_INVD                13
#define EXIT_REASON_INVLPG              14
#define EXIT_REASON_RDPMC               15
#define EXIT_REASON_RDTSC               16
#define EXIT_REASON_RSM                 17
#define EXIT_REASON_VMCALL              18
#define EXIT_REASON_VMCLEAR             19
#define EXIT_REASON_VMLAUNCH            20
#define EXIT_REASON_VMPTRLD             21
#define EXIT_REASON_VMPTRST             22
#define EXIT_REASON_VMREAD              23
#define EXIT_REASON_VMRESUME            24
#define EXIT_REASON_VMWRITE             25
#define EXIT_REASON_VMXOFF              26
#define EXIT_REASON_VMXON               27
#define EXIT_REASON_CR_ACCESS           28
#define EXIT_REASON_DR_ACCESS           29
#define EXIT_REASON_IO_INSTRUCTION      30
#define EXIT_REASON_MSR_READ            31
#define EXIT_REASON_MSR_WRITE           32
#define EXIT_REASON_INVALID_GUEST_STATE 33
#define EXIT_REASON_MSR_LOADING         34
#define EXIT_REASON_MWAIT_INSTRUCTION   36
#define EXIT_REASON_MONITOR_TRAP_FLAG   37
#define EXIT_REASON_MONITOR_INSTRUCTION 39
#define EXIT_REASON_PAUSE_INSTRUCTION   40
#define EXIT_REASON_MCE_DURING_VMENTRY  41
#define EXIT_REASON_TPR_BELOW_THRESHOLD 43
#define EXIT_REASON_APIC_ACCESS         44
#define EXIT_REASON_EPT_VIOLATION       48
#define EXIT_REASON_EPT_MISCONFIG       49
#define EXIT_REASON_RDTSCP              51
#define EXIT_REASON_WBINVD              54
#define EXIT_REASON_XSETBV              55

/*
 * Interruption-information format
 */
#define INTR_INFO_VECTOR_MASK           0xff            /* 7:0 */
#define INTR_INFO_INTR_TYPE_MASK        0x700           /* 10:8 */
#define INTR_INFO_DELIVER_CODE_MASK     0x800           /* 11 */
#define INTR_INFO_NMI_UNBLOCKED_BY_IRET 0x1000          /* 12 */
#define INTR_INFO_VALID_MASK            0x80000000      /* 31 */
#define INTR_INFO_RESVD_BITS_MASK       0x7ffff000

/*
 * Exit Qualifications for MOV for Control Register Access
 */
 /* 3:0 - control register number (CRn) */
#define VMX_CONTROL_REG_ACCESS_NUM      0xf
 /* 5:4 - access type (CR write, CR read, CLTS, LMSW) */
#define VMX_CONTROL_REG_ACCESS_TYPE     0x30
 /* 10:8 - general purpose register operand */
#define VMX_CONTROL_REG_ACCESS_GPR      0xf00
#define VMX_CONTROL_REG_ACCESS_TYPE_MOV_TO_CR   (0 << 4)
#define VMX_CONTROL_REG_ACCESS_TYPE_MOV_FROM_CR (1 << 4)
#define VMX_CONTROL_REG_ACCESS_TYPE_CLTS        (2 << 4)
#define VMX_CONTROL_REG_ACCESS_TYPE_LMSW        (3 << 4)
#define VMX_CONTROL_REG_ACCESS_GPR_EAX  (0 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_ECX  (1 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_EDX  (2 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_EBX  (3 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_ESP  (4 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_EBP  (5 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_ESI  (6 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_EDI  (7 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R8   (8 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R9   (9 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R10  (10 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R11  (11 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R12  (12 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R13  (13 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R14  (14 << 8)
#define VMX_CONTROL_REG_ACCESS_GPR_R15  (15 << 8)

/*
 * Access Rights
 */
#define X86_SEG_AR_SEG_TYPE     0xf        /* 3:0, segment type */
#define X86_SEG_AR_DESC_TYPE    (1u << 4)  /* 4, descriptor type */
#define X86_SEG_AR_DPL          0x60       /* 6:5, descriptor privilege level */
#define X86_SEG_AR_SEG_PRESENT  (1u << 7)  /* 7, segment present */
#define X86_SEG_AR_AVL          (1u << 12) /* 12, available for system software */
#define X86_SEG_AR_CS_LM_ACTIVE (1u << 13) /* 13, long mode active (CS only) */
#define X86_SEG_AR_DEF_OP_SIZE  (1u << 14) /* 14, default operation size */
#define X86_SEG_AR_GRANULARITY  (1u << 15) /* 15, granularity */
#define X86_SEG_AR_SEG_UNUSABLE (1u << 16) /* 16, segment unusable */

#define VMCALL_OPCODE   ".byte 0x0f,0x01,0xc1\n"
#define VMCLEAR_OPCODE  ".byte 0x66,0x0f,0xc7\n"        /* reg/opcode: /6 */
#define VMLAUNCH_OPCODE ".byte 0x0f,0x01,0xc2\n"
#define VMPTRLD_OPCODE  ".byte 0x0f,0xc7\n"             /* reg/opcode: /6 */
#define VMPTRST_OPCODE  ".byte 0x0f,0xc7\n"             /* reg/opcode: /7 */
#define VMREAD_OPCODE   ".byte 0x0f,0x78\n"
#define VMRESUME_OPCODE ".byte 0x0f,0x01,0xc3\n"
#define VMWRITE_OPCODE  ".byte 0x0f,0x79\n"
#define INVEPT_OPCODE   ".byte 0x66,0x0f,0x38,0x80\n"   /* m128,r64/32 */
#define INVVPID_OPCODE  ".byte 0x66,0x0f,0x38,0x81\n"   /* m128,r64/32 */
#define VMXOFF_OPCODE   ".byte 0x0f,0x01,0xc4\n"
#define VMXON_OPCODE    ".byte 0xf3,0x0f,0xc7\n"

#define MODRM_EAX_08    ".byte 0x08\n" /* ECX, [EAX] */
#define MODRM_EAX_06    ".byte 0x30\n" /* [EAX], with reg/opcode: /6 */
#define MODRM_EAX_07    ".byte 0x38\n" /* [EAX], with reg/opcode: /7 */
#define MODRM_EAX_ECX   ".byte 0xc1\n" /* EAX, ECX */

extern u64 vmx_ept_vpid_cap;

#define cpu_has_vmx_ept_wl4_supported           \
    (vmx_ept_vpid_cap & VMX_EPT_WALK_LENGTH_4_SUPPORTED)
#define cpu_has_vmx_ept_mt_uc                   \
    (vmx_ept_vpid_cap & VMX_EPT_MEMORY_TYPE_UC)
#define cpu_has_vmx_ept_mt_wb                   \
    (vmx_ept_vpid_cap & VMX_EPT_MEMORY_TYPE_WB)
#define cpu_has_vmx_ept_2mb                     \
    (vmx_ept_vpid_cap & VMX_EPT_SUPERPAGE_2MB)
#define cpu_has_vmx_ept_invept_single_context   \
    (vmx_ept_vpid_cap & VMX_EPT_INVEPT_SINGLE_CONTEXT)

#define INVEPT_SINGLE_CONTEXT   1
#define INVEPT_ALL_CONTEXT      2

#define cpu_has_vmx_vpid_invvpid_individual_addr                    \
    (vmx_ept_vpid_cap & VMX_VPID_INVVPID_INDIVIDUAL_ADDR)
#define cpu_has_vmx_vpid_invvpid_single_context                     \
    (vmx_ept_vpid_cap & VMX_VPID_INVVPID_SINGLE_CONTEXT)
#define cpu_has_vmx_vpid_invvpid_single_context_retaining_global    \
    (vmx_ept_vpid_cap & VMX_VPID_INVVPID_SINGLE_CONTEXT_RETAINING_GLOBAL)

#define INVVPID_INDIVIDUAL_ADDR                 0
#define INVVPID_SINGLE_CONTEXT                  1
#define INVVPID_ALL_CONTEXT                     2
#define INVVPID_SINGLE_CONTEXT_RETAINING_GLOBAL 3

static inline void __vmptrld(u64 addr)
{
    asm volatile ( VMPTRLD_OPCODE
                   MODRM_EAX_06
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 1f ; ud2 ; 1:\n"
                   :
                   : "a" (&addr)
                   : "memory");
}

static inline void __vmptrst(u64 addr)
{
    asm volatile ( VMPTRST_OPCODE
                   MODRM_EAX_07
                   :
                   : "a" (&addr)
                   : "memory");
}

static inline void __vmpclear(u64 addr)
{
    asm volatile ( VMCLEAR_OPCODE
                   MODRM_EAX_06
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 1f ; ud2 ; 1:\n"
                   :
                   : "a" (&addr)
                   : "memory");
}

static inline unsigned long __vmread(unsigned long field)
{
    unsigned long ecx;

    asm volatile ( VMREAD_OPCODE
                   MODRM_EAX_ECX
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 1f ; ud2 ; 1:\n"
                   : "=c" (ecx)
                   : "a" (field)
                   : "memory");

    return ecx;
}

static inline void __vmwrite(unsigned long field, unsigned long value)
{
    asm volatile ( VMWRITE_OPCODE
                   MODRM_EAX_ECX
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 1f ; ud2 ; 1:\n"
                   : 
                   : "a" (field) , "c" (value)
                   : "memory");
}

static inline unsigned long __vmread_safe(unsigned long field, int *error)
{
    unsigned long ecx;

    asm volatile ( VMREAD_OPCODE
                   MODRM_EAX_ECX
                   /* CF==1 or ZF==1 --> rc = -1 */
                   "setna %b0 ; neg %0"
                   : "=q" (*error), "=c" (ecx)
                   : "0" (0), "a" (field)
                   : "memory");

    return ecx;
}

static inline void __vm_set_bit(unsigned long field, unsigned int bit)
{
    __vmwrite(field, __vmread(field) | (1UL << bit));
}

static inline void __vm_clear_bit(unsigned long field, unsigned int bit)
{
    __vmwrite(field, __vmread(field) & ~(1UL << bit));
}

static inline void __invept(int type, u64 eptp, u64 gpa)
{
    struct {
        u64 eptp, gpa;
    } operand = {eptp, gpa};

    /*
     * If single context invalidation is not supported, we escalate to
     * use all context invalidation.
     */
    if ( (type == INVEPT_SINGLE_CONTEXT) &&
         !cpu_has_vmx_ept_invept_single_context )
        type = INVEPT_ALL_CONTEXT;

    asm volatile ( INVEPT_OPCODE
                   MODRM_EAX_08
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 1f ; ud2 ; 1:\n"
                   :
                   : "a" (&operand), "c" (type)
                   : "memory" );
}

static inline void __invvpid(int type, u16 vpid, u64 gva)
{
    struct {
        u64 vpid:16;
        u64 rsvd:48;
        u64 gva;
    } __attribute__ ((packed)) operand = {vpid, 0, gva};

    /* Fix up #UD exceptions which occur when TLBs are flushed before VMXON. */
    asm volatile ( "1: " INVVPID_OPCODE MODRM_EAX_08
                   /* CF==1 or ZF==1 --> crash (ud2) */
                   "ja 2f ; ud2 ; 2:\n"
                   ".section __ex_table,\"a\"\n"
                   "    "__FIXUP_ALIGN"\n"
                   "    "__FIXUP_WORD" 1b,2b\n"
                   ".previous"
                   :
                   : "a" (&operand), "c" (type)
                   : "memory" );
}

static inline void ept_sync_all(void)
{
    __invept(INVEPT_ALL_CONTEXT, 0, 0);
}

static inline void vpid_sync_vcpu_gva(struct vm_info *vm, unsigned long gva)
{
    int type = INVVPID_INDIVIDUAL_ADDR;

    /*
     * If individual address invalidation is not supported, we escalate to
     * use single context invalidation.
     */
    if ( likely(cpu_has_vmx_vpid_invvpid_individual_addr) )
        goto execute_invvpid;

    type = INVVPID_SINGLE_CONTEXT;

    /*
     * If single context invalidation is not supported, we escalate to
     * use all context invalidation.
     */
    if ( !cpu_has_vmx_vpid_invvpid_single_context )
        type = INVVPID_ALL_CONTEXT;

execute_invvpid:
    __invvpid(type, vm->asid, (u64)gva);
}

static inline void vpid_sync_all(void)
{
    __invvpid(INVVPID_ALL_CONTEXT, 0, 0);
}

static inline void __vmxoff(void)
{
    asm volatile (
        VMXOFF_OPCODE
        : : : "memory" );
}

static inline int __vmxon(u64 addr)
{
    int rc;

    asm volatile ( 
        "1: " VMXON_OPCODE MODRM_EAX_06 "\n"
        "   setna %b0 ; neg %0\n" /* CF==1 or ZF==1 --> rc = -1 */
        "2:\n"
        ".section .fixup,\"ax\"\n"
        "3: sub $2,%0 ; jmp 2b\n"    /* #UD or #GP --> rc = -2 */
        ".previous\n"
        ".section __ex_table,\"a\"\n"
        "   "__FIXUP_ALIGN"\n"
        "   "__FIXUP_WORD" 1b,3b\n"
        ".previous\n"
        : "=q" (rc)
        : "0" (0), "a" (&addr)
        : "memory");

    return rc;
}

/* EPT violation qualifications definitions */
#define _EPT_READ_VIOLATION         0
#define EPT_READ_VIOLATION          (1UL<<_EPT_READ_VIOLATION)
#define _EPT_WRITE_VIOLATION        1
#define EPT_WRITE_VIOLATION         (1UL<<_EPT_WRITE_VIOLATION)
#define _EPT_EXEC_VIOLATION         2
#define EPT_EXEC_VIOLATION          (1UL<<_EPT_EXEC_VIOLATION)
#define _EPT_EFFECTIVE_READ         3
#define EPT_EFFECTIVE_READ          (1UL<<_EPT_EFFECTIVE_READ)
#define _EPT_EFFECTIVE_WRITE        4
#define EPT_EFFECTIVE_WRITE         (1UL<<_EPT_EFFECTIVE_WRITE)
#define _EPT_EFFECTIVE_EXEC         5
#define EPT_EFFECTIVE_EXEC          (1UL<<_EPT_EFFECTIVE_EXEC)
#define _EPT_GLA_VALID              7
#define EPT_GLA_VALID               (1UL<<_EPT_GLA_VALID)
#define _EPT_GLA_FAULT              8
#define EPT_GLA_FAULT               (1UL<<_EPT_GLA_FAULT)

#define EPT_PAGETABLE_ENTRIES       512

extern void __init init_intel( struct vm_info *vm, struct cpuinfo_x86 *cpuinfo );

#endif /* __VMX_H__ */
