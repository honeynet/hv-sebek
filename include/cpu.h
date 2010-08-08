#ifndef __CPU_H__
#define __CPU_H__

#include "compiler.h"
#include "types.h"
#include "cpufeature.h"

/*
 * CPU vendor IDs
 */
#define X86_VENDOR_INTEL 0
#define X86_VENDOR_CYRIX 1
#define X86_VENDOR_AMD 2
#define X86_VENDOR_UMC 3
#define X86_VENDOR_NEXGEN 4
#define X86_VENDOR_CENTAUR 5
#define X86_VENDOR_RISE 6
#define X86_VENDOR_TRANSMETA 7
#define X86_VENDOR_NSC 8
#define X86_VENDOR_NUM 9
#define X86_VENDOR_UNKNOWN 0xff

/*
 * EFLAGS bits
 */
#define X86_RFLAGS_CF	0x00000001 /* Carry Flag */
#define X86_RFLAGS_PF	0x00000004 /* Parity Flag */
#define X86_RFLAGS_AF	0x00000010 /* Auxillary carry Flag */
#define X86_RFLAGS_ZF	0x00000040 /* Zero Flag */
#define X86_RFLAGS_SF	0x00000080 /* Sign Flag */
#define X86_RFLAGS_TF	0x00000100 /* Trap Flag */
#define X86_RFLAGS_IF	0x00000200 /* Interrupt Flag */
#define X86_RFLAGS_DF	0x00000400 /* Direction Flag */
#define X86_RFLAGS_OF	0x00000800 /* Overflow Flag */
#define X86_RFLAGS_IOPL	0x00003000 /* IOPL mask */
#define X86_RFLAGS_NT	0x00004000 /* Nested Task */
#define X86_RFLAGS_RF	0x00010000 /* Resume Flag */
#define X86_RFLAGS_VM	0x00020000 /* Virtual Mode */
#define X86_RFLAGS_AC	0x00040000 /* Alignment Check */
#define X86_RFLAGS_VIF	0x00080000 /* Virtual Interrupt Flag */
#define X86_RFLAGS_VIP	0x00100000 /* Virtual Interrupt Pending */
#define X86_RFLAGS_ID	0x00200000 /* CPUID detection flag */

/*
 * Intel CPU flags in CR0
 */
#define X86_CR0_PE              0x00000001 /* Enable Protected Mode    (RW) */
#define X86_CR0_MP              0x00000002 /* Monitor Coprocessor      (RW) */
#define X86_CR0_EM              0x00000004 /* Require FPU Emulation    (RO) */
#define X86_CR0_TS              0x00000008 /* Task Switched            (RW) */
#define X86_CR0_ET              0x00000010 /* Extension type           (RO) */
#define X86_CR0_NE              0x00000020 /* Numeric Error Reporting  (RW) */
#define X86_CR0_WP              0x00010000 /* Supervisor Write Protect (RW) */
#define X86_CR0_AM              0x00040000 /* Alignment Checking       (RW) */
#define X86_CR0_NW              0x20000000 /* Not Write-Through        (RW) */
#define X86_CR0_CD              0x40000000 /* Cache Disable            (RW) */
#define X86_CR0_PG              0x80000000 /* Paging                   (RW) */

/*
 * Intel CPU features in CR4
 */
#define X86_CR4_VME		0x0001	/* enable vm86 extensions */
#define X86_CR4_PVI		0x0002	/* virtual interrupts flag enable */
#define X86_CR4_TSD		0x0004	/* disable time stamp at ipl 3 */
#define X86_CR4_DE		0x0008	/* enable debugging extensions */
#define X86_CR4_PSE		0x0010	/* enable page size extensions */
#define X86_CR4_PAE		0x0020	/* enable physical address extensions */
#define X86_CR4_MCE		0x0040	/* Machine check enable */
#define X86_CR4_PGE		0x0080	/* enable global pages */
#define X86_CR4_PCE		0x0100	/* enable performance counters at ipl 3 */
#define X86_CR4_OSFXSR		0x0200	/* enable fast FPU save and restore */
#define X86_CR4_OSXMMEXCPT	0x0400	/* enable unmasked SSE exceptions */
#define X86_CR4_VMXE		0x2000  /* enable VMX */
#define X86_CR4_SMXE		0x4000  /* enable SMX */
#define X86_CR4_OSXSAVE	0x40000 /* enable XSAVE/XRSTOR */

/*
 * Trap/fault mnemonics.
 */
#define TRAP_divide_error      0
#define TRAP_debug             1
#define TRAP_nmi               2
#define TRAP_int3              3
#define TRAP_overflow          4
#define TRAP_bounds            5
#define TRAP_invalid_op        6
#define TRAP_no_device         7
#define TRAP_double_fault      8
#define TRAP_copro_seg         9
#define TRAP_invalid_tss      10
#define TRAP_no_segment       11
#define TRAP_stack_error      12
#define TRAP_gp_fault         13
#define TRAP_page_fault       14
#define TRAP_spurious_int     15
#define TRAP_copro_error      16
#define TRAP_alignment_check  17
#define TRAP_machine_check    18
#define TRAP_simd_error       19

/* Set for entry via SYSCALL. Informs return code to use SYSRETQ not IRETQ. */
/* NB. Same as VGCF_in_syscall. No bits in common with any other TRAP_ defn. */
#define TRAP_syscall         256

/* Boolean return code: the reason for a fault has been fixed. */
#define EXCRET_fault_fixed 1

/* 'trap_bounce' flags values */
#define TBF_EXCEPTION          1
#define TBF_EXCEPTION_ERRCODE  2
#define TBF_INTERRUPT          8
#define TBF_FAILSAFE          16

/* 'arch_vcpu' flags values */
#define _TF_kernel_mode        0
#define TF_kernel_mode         (1<<_TF_kernel_mode)

/* #PF error code values. */
#define PFEC_page_present   (1U<<0)
#define PFEC_write_access   (1U<<1)
#define PFEC_user_mode      (1U<<2)
#define PFEC_reserved_bit   (1U<<3)
#define PFEC_insn_fetch     (1U<<4)
#define PFEC_page_paged     (1U<<5)
#define PFEC_page_shared    (1U<<6)

#define X86_DR6_BS			(1U<<14)

#define __FIXUP_ALIGN ".align 8" 
#define __FIXUP_WORD  ".quad"

#ifndef __ASSEMBLY__

#define __read_mostly __attribute__((__section__(".data.read_mostly")))

struct cpuinfo_x86 {
	u8		x86;		/* CPU family */
	u8		x86_vendor;	/* CPU vendor */
	u8		x86_model;
	u8		x86_mask;
	
	u32		cpuid_max_stdfunc;	/* Maximum supported CPUID standard function, -1 = no CPUID */
	u32		cpuid_max_exfunc;	/* Max extended CPUID function supported */
	u32		x86_capability[NCAPINTS];

	char	x86_vendor_id[16];
	char	x86_model_id[64];
	
	int 	x86_cache_size;  /* in KB */
	int		x86_cache_alignment;
	u16		x86_clflush_size;
	int		x86_tlbsize;	/* number of 4K pages in DTLB/ITLB combined(in pages)*/
    
	u8		x86_virt_bits;
	u8		x86_phys_bits;
    u32		x86_power;
	
	u16		apicid;
	u16		initial_apicid;

	unsigned long loops_per_jiffy;
	
	u16		x86_max_cores;	/* cpuid returned max cores value */
	u16		booted_cores;	/* number of cores as seen by OS */
	u16		phys_proc_id;	/* Physical processor id: */
	u16		cpu_core_id;	/* Core id: */
} __attribute__((__aligned__(1 << 7)));

/*
 * Attribute for segment selector. This is a copy of bit 40:47 & 52:55 of the
 * segment descriptor. */
union seg_attrs
{
	//Anh - segment attributes is a concatenation of bits 40 - 47 and 52-55 of descriptor entry
	u16 bytes;
	struct {
		u16 type:4;    /* 0;  Bit 40-43 */
		u16 s:   1;    /* 4;  Bit 44 */
		u16 dpl: 2;    /* 5;  Bit 45-46 */
		u16 p:   1;    /* 7;  Bit 47 */
		u16 avl: 1;    /* 8;  Bit 52 */
		u16 l:   1;    /* 9;  Bit 53 */
		u16 db:  1;    /* 10; Bit 54 */
		u16 g:   1;    /* 11; Bit 55 */
	} fields;
} __attribute__ ((packed));

struct seg_selector {
	u16        	sel;
	union seg_attrs	attrs;
	u32        	limit;
	u64        	base;
} __attribute__ ((packed));

/*
 * Generic CPUID function
 * clear %ecx since some cpus (Cyrix MII) do not set or clear %ecx
 * resulting in stale register contents being returned.
 */
static always_inline void
cpuid( unsigned int op, unsigned int *eax, unsigned int *ebx,
    unsigned int *ecx, unsigned int *edx )
{
	*ecx = 0;

	asm volatile("cpuid"
		: "=a" (*eax),
		  "=b" (*ebx),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "0" (op), "2" (0) );
}


/* Some CPUID calls want 'count' to be placed in ecx */
static inline void cpuid_count(
    int op,
    int count,
    unsigned int *eax,
    unsigned int *ebx,
    unsigned int *ecx,
    unsigned int *edx)
{
    asm ( "cpuid"
          : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
          : "0" (op), "c" (count) );
}

/*
 * CPUID functions returning a single datum
 */
static always_inline unsigned int cpuid_eax(unsigned int op)
{
	unsigned int eax, ebx, ecx, edx;

	cpuid(op, &eax, &ebx, &ecx, &edx);

	return eax;
}

static always_inline unsigned int cpuid_ebx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    cpuid(op, &eax, &ebx, &ecx, &edx);

	return ebx;
}

static always_inline unsigned int cpuid_ecx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    cpuid(op, &eax, &ebx, &ecx, &edx);

	return ecx;
}

static always_inline unsigned int cpuid_edx(unsigned int op)
{
    unsigned int eax, ebx, ecx, edx;

    cpuid(op, &eax, &ebx, &ecx, &edx);

	return edx;
}

static inline unsigned long read_cr0(void)
{
    unsigned long cr0;
    asm volatile ( "movq %%cr0,%0\n\t" : "=r" (cr0) );
    return cr0;
} 

static inline void write_cr0(unsigned long val)
{
    asm volatile ( "movq %0,%%cr0" : : "r" (val) );
}

static inline unsigned long read_cr2(void)
{
    unsigned long cr2;
    asm volatile ( "movq %%cr2,%0\n\t" : "=r" (cr2) );
    return cr2;
}

static inline unsigned long read_cr4(void)
{
	unsigned long cr4;
	asm volatile ( "movq %%cr4,%0\n\t" : "=r" (cr4) );
	return cr4;
}

static inline void write_cr4(unsigned long val)
{
    asm volatile ( "movq %0,%%cr4" : : "r" (val) );
}

/* Clear and set 'TS' bit respectively */
static inline void clts(void) 
{
    asm volatile ( "clts" );
}

static inline void stts(void) 
{
    write_cr0(X86_CR0_TS|read_cr0());
}

/*
 * Save the cr4 feature set we're using (ie
 * Pentium 4MB enable and PPro Global page
 * enable), so that any CPU's that boot up
 * after us can get the correct flags.
 */
extern unsigned long mmu_cr4_features;

static always_inline void set_in_cr4 (unsigned long mask)
{
    //mmu_cr4_features |= mask;
    write_cr4(read_cr4() | mask);
}

static always_inline void clear_in_cr4 (unsigned long mask)
{
    //mmu_cr4_features &= ~mask;
    write_cr4(read_cr4() & ~mask);
}

/* Interrupt Descriptor Table Register (IDTR)
 *
 * The content of the IDTR is copied by sidt to the six
 * bytes of memory specified by the operand. The first word
 * at the effective address is assigned the LIMIT field of
 * the register. If the operand-size attribute is 16-bit,
 * the 32-bit BASE field of the register is assigned to the
 * next four bytes.
 *
 * MSB of the IDT base address of a native system: 0xc0
 */
static inline unsigned long get_idt_base (void)
{
	unsigned char	idtr[6];
	unsigned long	idt;

	asm volatile ("sidt %0" : "=m" (idtr));
	idt = *((unsigned long *) &idtr[2]);

	return (idt);
}

static inline unsigned short get_idt_limit (void)
{
	unsigned char	idtr[6];
	unsigned long	idt;

	asm volatile ("sidt %0" : "=m" (idtr));
	idt = *((unsigned short *) &idtr[0]);

	return (idt);
}

extern void early_identify_cpu ( struct cpuinfo_x86 *c );
extern void display_cacheinfo ( struct cpuinfo_x86 *c );

#endif /* __ASSEMBLY__ */

#endif /* __CPU_H__ */
