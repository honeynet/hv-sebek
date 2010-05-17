#ifndef __VMEXIT_H__
#define __VMEXIT_H__

#include "vmcb.h"

/* [REF] AMD64 manual Vol. 2, Appendix C */


enum vmexit_exitcode {
    /* control register read exitcodes */
    VMEXIT_CR0_READ    =  0x0,
    VMEXIT_CR1_READ    =  0x1,
    VMEXIT_CR2_READ    =  0x2,
    VMEXIT_CR3_READ    =  0x3,
    VMEXIT_CR4_READ    =  0x4,
    VMEXIT_CR5_READ    =  0x5,
    VMEXIT_CR6_READ    =  0x6,
    VMEXIT_CR7_READ    =  0x7,
    VMEXIT_CR8_READ    =  0x8,
    VMEXIT_CR9_READ    =  0x9,
    VMEXIT_CR10_READ   =  0xA,
    VMEXIT_CR11_READ   =  0xB,
    VMEXIT_CR12_READ   =  0xC,
    VMEXIT_CR13_READ   =  0xD,
    VMEXIT_CR14_READ   =  0xE,
    VMEXIT_CR15_READ   =  0xF,

    /* control register write exitcodes */
    VMEXIT_CR0_WRITE   =  0x10,
    VMEXIT_CR1_WRITE   =  0x11,
    VMEXIT_CR2_WRITE   =  0x12,
    VMEXIT_CR3_WRITE   =  0x13,
    VMEXIT_CR4_WRITE   =  0x14,
    VMEXIT_CR5_WRITE   =  0x15,
    VMEXIT_CR6_WRITE   =  0x16,
    VMEXIT_CR7_WRITE   =  0x17,
    VMEXIT_CR8_WRITE   =  0x18,
    VMEXIT_CR9_WRITE   =  0x19,
    VMEXIT_CR10_WRITE  =  0x1A,
    VMEXIT_CR11_WRITE  =  0x1B,
    VMEXIT_CR12_WRITE  =  0x1C,
    VMEXIT_CR13_WRITE  =  0x1D,
    VMEXIT_CR14_WRITE  =  0x1E,
    VMEXIT_CR15_WRITE  =  0x1F,

    /* debug register read exitcodes */
    VMEXIT_DR0_READ    =  0x20,
    VMEXIT_DR1_READ    =  0x21,
    VMEXIT_DR2_READ    =  0x22,
    VMEXIT_DR3_READ    =  0x23,
    VMEXIT_DR4_READ    =  0x24,
    VMEXIT_DR5_READ    =  0x25,
    VMEXIT_DR6_READ    =  0x26,
    VMEXIT_DR7_READ    =  0x27,
    VMEXIT_DR8_READ    =  0x28,
    VMEXIT_DR9_READ    =  0x29,
    VMEXIT_DR10_READ   =  0x2A,
    VMEXIT_DR11_READ   =  0x2B,
    VMEXIT_DR12_READ   =  0x2C,
    VMEXIT_DR13_READ   =  0x2D,
    VMEXIT_DR14_READ   =  0x2E,
    VMEXIT_DR15_READ   =  0x2F,

    /* debug register write exitcodes */
    VMEXIT_DR0_WRITE   =  0x30,
    VMEXIT_DR1_WRITE   =  0x31,
    VMEXIT_DR2_WRITE   =  0x32,
    VMEXIT_DR3_WRITE   =  0x33,
    VMEXIT_DR4_WRITE   =  0x34,
    VMEXIT_DR5_WRITE   =  0x35,
    VMEXIT_DR6_WRITE   =  0x36,
    VMEXIT_DR7_WRITE   =  0x37,
    VMEXIT_DR8_WRITE   =  0x38,
    VMEXIT_DR9_WRITE   =  0x39,
    VMEXIT_DR10_WRITE  =  0x3A,
    VMEXIT_DR11_WRITE  =  0x3B,
    VMEXIT_DR12_WRITE  =  0x3C,
    VMEXIT_DR13_WRITE  =  0x3D,
    VMEXIT_DR14_WRITE  =  0x3E,
    VMEXIT_DR15_WRITE  =  0x3F,

    /* processor exception exitcodes (VMEXIT_EXCP[0-31]) */
    VMEXIT_EXCEPTION_DE  =  0x40, /* divide-by-zero-error */
    VMEXIT_EXCEPTION_DB  =  0x41, /* debug */
    VMEXIT_EXCEPTION_NMI =  0x42, /* non-maskable-interrupt */
    VMEXIT_EXCEPTION_BP  =  0x43, /* breakpoint */
    VMEXIT_EXCEPTION_OF  =  0x44, /* overflow */
    VMEXIT_EXCEPTION_BR  =  0x45, /* bound-range */
    VMEXIT_EXCEPTION_UD  =  0x46, /* invalid-opcode*/
    VMEXIT_EXCEPTION_NM  =  0x47, /* device-not-available */
    VMEXIT_EXCEPTION_DF  =  0x48, /* double-fault */
    VMEXIT_EXCEPTION_09  =  0x49, /* unsupported (reserved) */
    VMEXIT_EXCEPTION_TS  =  0x4A, /* invalid-tss */
    VMEXIT_EXCEPTION_NP  =  0x4B, /* segment-not-present */
    VMEXIT_EXCEPTION_SS  =  0x4C, /* stack */
    VMEXIT_EXCEPTION_GP  =  0x4D, /* general-protection */
    VMEXIT_EXCEPTION_PF  =  0x4E, /* page-fault */
    VMEXIT_EXCEPTION_15  =  0x4F, /* reserved */
    VMEXIT_EXCEPTION_MF  =  0x50, /* x87 floating-point exception-pending */
    VMEXIT_EXCEPTION_AC  =  0x51, /* alignment-check */
    VMEXIT_EXCEPTION_MC  =  0x52, /* machine-check */
    VMEXIT_EXCEPTION_XF  =  0x53, /* simd floating-point */

    /* exceptions 20-31 (exitcodes 84-95) are reserved */

    /* ...and the rest of the #VMEXITs */
    VMEXIT_INTR             = 0x60,
    VMEXIT_NMI              = 0x61,
    VMEXIT_SMI              = 0x62,
    VMEXIT_INIT             = 0x63,
    VMEXIT_VINTR            = 0x64,
    VMEXIT_CR0_SEL_WRITE    = 0x65,
    VMEXIT_IDTR_READ        = 0x66,
    VMEXIT_GDTR_READ        = 0x67,
    VMEXIT_LDTR_READ        = 0x68,
    VMEXIT_TR_READ          = 0x69,
    VMEXIT_IDTR_WRITE       = 0x6A,
    VMEXIT_GDTR_WRITE       = 0x6B,
    VMEXIT_LDTR_WRITE       = 0x6C,
    VMEXIT_TR_WRITE         = 0x6D,
    VMEXIT_RDTSC            = 0x6E,
    VMEXIT_RDPMC            = 0x6F,
    VMEXIT_PUSHF            = 0x70,
    VMEXIT_POPF             = 0x71,
    VMEXIT_CPUID            = 0x72,
    VMEXIT_RSM              = 0x73,
    VMEXIT_IRET             = 0x74,
    VMEXIT_SWINT            = 0x75,	//software interrupt
    VMEXIT_INVD             = 0x76,
    VMEXIT_PAUSE            = 0x77,
    VMEXIT_HLT              = 0x78,
    VMEXIT_INVLPG           = 0x79,
    VMEXIT_INVLPGA          = 0x7A,
    VMEXIT_IOIO             = 0x7B,
    VMEXIT_MSR              = 0x7C,
    VMEXIT_TASK_SWITCH      = 0x7D,
    VMEXIT_FERR_FREEZE      = 0x7E,
    VMEXIT_SHUTDOWN         = 0x7F,
    VMEXIT_VMRUN            = 0x80,
    VMEXIT_VMMCALL          = 0x81,
    VMEXIT_VMLOAD           = 0x82,
    VMEXIT_VMSAVE           = 0x83,
    VMEXIT_STGI             = 0x84,
    VMEXIT_CLGI             = 0x85,
    VMEXIT_SKINIT           = 0x86,
    VMEXIT_RDTSCP           = 0x87,
    VMEXIT_ICEBP            = 0x88,
    VMEXIT_NPF              = 0x400, /* nested paging fault */
    VMEXIT_INVALID          =  -1
};


extern void print_vmexit_exitcode ( struct vmcb * );

#endif /* __VMEXIT_H__ */
