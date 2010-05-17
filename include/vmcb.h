#ifndef __VMCB_H__
#define __VMCB_H__

#include "types.h"

#define MSR_SYSENTER_CS		0x174
#define MSR_SYSENTER_ESP	0x175
#define MSR_SYSENTER_EIP	0x176

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

union eventinj {
	u64 bytes;
	struct {
		u64 vector:    8;
		u64 type:      3;
		u64 ev:        1;
		u64 resvd1:   19;
		u64 v:         1;
		u64 errorcode:32;
	} fields;
} __attribute__ ((packed));

enum EVENT_TYPES
{
	EVENT_TYPE_INTR = 0,
	EVENT_TYPE_NMI = 2,
	EVENT_TYPE_EXCEPTION = 3,
	EVENT_TYPE_SWINT = 4,
};

union vintr
{
	u64 bytes;
	struct {
        u64 tpr:          8;
        u64 irq:          1;
        u64 rsvd0:        7;
        u64 prio:         4;
        u64 ign_tpr:      1;
        u64 rsvd1:        3;
        u64 intr_masking: 1;
        u64 rsvd2:        7;
        u64 vector:       8;
        u64 rsvd3:       24;
    } fields;
} __attribute__ ((packed));

struct vmcb
{
	/*** Control Area ***/
	u32 cr_intercepts;          /* offset 0x00 */
	u32 dr_intercepts;          /* offset 0x04 */
	u32 exception_intercepts;   /* offset 0x08 */
	u32 general1_intercepts;    /* offset 0x0C */
	u32 general2_intercepts;    /* offset 0x10 */
	u32 res01;                  /* offset 0x14 */
	u64 res02;                  /* offset 0x18 */
	u64 res03;                  /* offset 0x20 */
	u64 res04;                  /* offset 0x28 */
	u64 res05;                  /* offset 0x30 */
	u64 res06;                  /* offset 0x38 */
	u64 iopm_base_pa;           /* offset 0x40 */
	u64 msrpm_base_pa;          /* offset 0x48 */
	u64 tsc_offset;             /* offset 0x50 */
	u32 guest_asid;             /* offset 0x58 */
	u8  tlb_control;            /* offset 0x5C */
	u8  res07[3];
	union vintr vintr;              /* offset 0x60 */
	u64 interrupt_shadow;       /* offset 0x68 */
	u64 exitcode;               /* offset 0x70 */
	u64 exitinfo1;              /* offset 0x78 */
	u64 exitinfo2;              /* offset 0x80 */
	union eventinj exitintinfo;    /* offset 0x88 */
	u64 np_enable;              /* offset 0x90 */   /* set 1 to enable nested page table */
	u64 res08[2];
	union eventinj eventinj;       /* offset 0xA8 */
	u64 n_cr3;                  /* offset 0xB0 */   /* physical memory of the VM --> physical memory of the PM */
	u64 res09[105];             /* offset 0xB8 pad to save area */

	/*** State Save Area ****/
	struct seg_selector es, cs, ss, ds, fs, gs, gdtr, ldtr, idtr, tr;      /* offset 1024 */
	u64 res10[5];
	u8 res11[3];
	u8 cpl;
	u32 res12;
	u64 efer;               	/* offset 1024 + 0xD0 */
	u64 res13[14];
	u64 cr4;                  	/* loffset 1024 + 0x148 */
	u64 cr3;
	u64 cr0;
	u64 dr7;
	u64 dr6;
	u64 rflags;
	u64 rip;
	u64 res14[11]; /* reserved */
	u64 rsp;
	u64 res15[3]; /* reserved */
	u64 rax;
	u64 star;
	u64 lstar;
	u64 cstar;
	u64 sfmask;
	u64 kerngsbase;
	u64 sysenter_cs;
	u64 sysenter_esp;
	u64 sysenter_eip;
	u64 cr2;
	u64 pdpe0; /* reserved ? */
	u64 pdpe1; /* reserved ? */
	u64 pdpe2; /* reserved ? */
	u64 pdpe3; /* reserved ? */
	u64 g_pat;
	u64 res16[50];
	u64 res17[128];
	u64 res18[128];
} __attribute__ ((packed));

extern void vmcb_check_consistency ( struct vmcb *vmcb );
extern void vmcb_dump( const struct vmcb *vmcb);
extern void print_vmcb_state (struct vmcb *vmcb);

#endif /* __VMCB_H__ */
