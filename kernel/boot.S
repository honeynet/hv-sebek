/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#define __ASSEMBLY__

#include "multiboot.h"
#include "cpu.h"
#include "msr.h"
#include "page.h"
#include "vmm.h"

#define CPU_ID_EXT_FEATURES	0

	.text
	.code32

	.globl	start, _start

start:
_start:
	jmp	physical_entry /* NOTE: relative addressing */

	/* Anh - breakpoint: mov 0xffffff, %dl - set in Simnow using "BM 0xffffff", read chapter 10, Simnow Manual */
breakpoint:
	pushl 	%edx
	mov	0xffffff, %dl
	popl 	%edx
	ret

die:
	ud2

physical_entry:
	cld		/* clear direction flag - x86 opcode ref: http://pdos.csail.mit.edu/6.828/2008/readings/i386/c17.htm */
	cli		/* clear interrupt flag - mask external interrupts. */

			/* When should re re-enable?
			 * check multiboot specification for machine state after loader:
			 * http://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html#Machine-state
			 * The OS image must leave interrupts disabled until it sets up its own IDT
			 */

	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC, %eax	/* eax should be 0x2BADB002 after receiving control from GRUB */
	jne	die					/* Otherwise => some error */

	/* ebx contains pointer to multibook structure, describing machine configurations, command line, etc...
	 * We save the multiboot info structure for later use.
	 * check http://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
	 */
	movl	%ebx, PHYS(multiboot_ptr)

	/************************** Set up DATA Descriptors.**************************/
 	lgdt 	PHYS(boot_gdt_descr)		/* Initial value for GDTR, point to a handcrafted GDT */
	movl 	$VMM_DS32, %eax			/* All data segment registers point to the entry 2 of GDT */
	movl 	%eax, %ds			/* (read format of segment registers in AMD manual vol2) */
	movl 	%eax, %es
	movl 	%eax, %ss

	/* Set up FPU.  */
	fninit

	/************ Enabling Paging...  **********************************************/

	/* Enable PAE & PSE in CR4, 64bit for paging */
	movl	$(X86_CR4_PAE | X86_CR4_PSE), %ecx
	movl	%ecx, %cr4

	/* Load pagetable base register.  */
	movl	$(PHYS(vmm_pml4_table)), %eax
	movl	%eax, %cr3

	/******************* Set up EFER (Extended Feature Enable Register).  *********************/
	movl	$MSR_EFER, %ecx		/* set ecx = indentifier of EFER MSR (machine specific register) */
	rdmsr				/* read value of EFER MSR to eax */

	/* Enable long mode */
	btsl	$_EFER_LME, %eax

	/* Enable system call extension (no SYSCALL, SYSRET) */
	btsl	$_EFER_SCE, %eax	//Set bit 0

	movl	$CPU_ID_EXT_FEATURES, %edi	/* CPU_ID_EXT_FEATURES = 0 */
	btl	$20, %edi			/* CPUID 0x80000001, EDX[20] */
	jnc	1f
	btsl    $_EFER_NX, %eax			/* Anh - enable no execute page-protection feature */
1:      wrmsr					/* write eax to EFER MSR */

	/***************** Set initial value for CR0 (See AMD64 manual vol. 2, p. 42).  ******************/
	/*movl	$(X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_AM | X86_CR0_PG), %eax*/
	movl	$(X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_WP | X86_CR0_AM | X86_CR0_PG), %eax
	movl	%eax, %cr0	

	/* Paging enabled.  Now in compatibility mode. */

	/* Long-jump into 64-bit mode. (See AMD64 manual Vol. 2, p. 104) */ 
	ljmp	$(VMM_CS64), $(PHYS(virtual_entry)) /* PHYS(...) is used to calculate the offset of the target address into the code segment */

	.code64
virtual_entry:
	/* Install relocated selectors (FS/GS unused). */
	lgdt	gdt_descr(%rip) /* [Note] RIP-Relative Addressing (See AMD64 manual Vol. 3, p. 23) */

	/* Initialize the stack.  */
	movabs	$(boot_stack + STACK_SIZE), %rsp
	movq    %rsp, %rbp

	/* Reset EFLAGS (including interupt and direction flags). */
	pushq	$0
	popfq		//now eflag = 0

	/* [???] In data segment descriptor of long-mode, only present-bit is valid. */
	movl	$(VMM_DS64), %ecx /* ??? */
	movl	%ecx, %ds
	movl	%ecx, %es
	movl	%ecx, %fs
	movl	%ecx, %gs
	movl	%ecx, %ss

	/* Pass off the Multiboot info structure to C land (move its address into %edi). */
	/* Chengyu - long mode does not use stack to pass arg0 */
	
	movl	multiboot_ptr(%rip), %edi
	movq	$0, %rax
	addq	%rax, %rdi


	/* passing of multiboot_ptr argument checked - OK
	 * jump too start_vmm function, which is defined in setup.c 
	 */
	call    start_vmm

	/* Force a panic (the program should not reach this point of the code).  */
	ud2

/********************************************************************************************/
/************************************ DATA **************************************************/
/********************************************************************************************/

	.data
	.code32
	/* Note: .hword = 16 bits (2 bytes), .long = 32 bits (4 bytes), .quad = 64 bits (8 bytes) */

	/*
	 * Multiboot header. To checked by GRUB loader
	 * The Multiboot header must appear within the first 8192 bytes of the OS image,
	 * and must be longword (32-bit) aligned. In general, it should come as early as possible
	 *
	 * check http://www.gnu.org/software/grub/manual/multiboot/html_node/Header-layout.html#Header-layout
	 * and http://www.gnu.org/software/grub/manual/multiboot/html_node/OS-image-format.html#OS-image-format
	 */

	.align	4	/* Align 32 bits boundary.  */
multiboot_header:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) /* checksum */

/************************************ Descriptors **************************************************/
	/* Initial selector for GDTR - AMD vol2 p72*/
	.align	4
boot_gdt_descr:
	.word	GDT_ENTRIES * 8 - 1  /* limit */
	.quad	PHYS(gdt_table)  /* address of GDT (when paging is disabled) */

gdt_descr:
	.word	GDT_ENTRIES * 8 - 1 /* limit */
	.quad	gdt_table	/* address of GDT (when paging is enabled) */

	/* Initial Global Descriptor Table (See AMD64 vol. 2 p78) */
	.align	PAGE_SIZE
gdt_table:
	.quad	0x0000000000000000     	/* null selector */

	.quad	0x00cf9e000000ffff     	/* ring 0 (dpl = 0), code, compatibility
					 * TODO - Conforming = 1 or 0? vol2 p80
			          	 * (base = 0, limit = fffff, G (limit x 4KB), R, P, D (32 bits operands) */
			          	/* 11001111 [P=1][DPL=00][S=1][T=1110] 00000000 00000000 00000000 11111111 11111111 */
	
	.quad	0x00cf92000000ffff     	/* ring 0, data */
    					/*11001111 [P=1][DPL=00][S=1][T=0010] 00000000 00000000 00000000 11111111 11111111 */

	.quad	0x00cffe000000ffff     	/* ring 3 code, compatibility */
    					/* 11001111 [P=1][DPL=11][S=1][T=1110] 00000000 00000000 00000000 11111111 11111111 */

	.quad	0x00cff2000000ffff     	/* ring 3 data */
    					/* 11001111 [P=1][DPL=11][S=1][T=0010] 00000000 00000000 00000000 11111111 11111111 */

	.quad	0x00af9e000000ffff	/* ring 0 code, 64-bit mode 
					 * (base = 0, limit = fffff, G (ignored), C, R, P, L (64 bits operands) */
	
	.quad	0x00affe000000ffff	/* ring 3 code, 64-bit mode
					 * (base = 0, limit = fffff, G (ignored), C, R, P, L (64 bits operands) */

	.quad	0x0000800000000000	/* ring 0/3 data, 64-bit mode (present only) */

	.fill	4, 8, 0			/* space for TSS and LDT per CPU */


/************************************ Paging *******************************************************/
	/* check http://sourceware.org/binutils/docs-2.18/as/Macro.html#Macro */
	.macro  identmap from, to
		.quad \from * 0x200000 | PTTEF_PRESENT | PTTEF_RW | PTTEF_PAGE_SIZE | PTTEF_US	/* 0x200000 = 2MB */
		.if     \to-\from
			identmap "(\from+1)",\to	/* recursive function */
		.endif
	.endm

	/* identical map, but read only */
	.macro  mapro from, to
		.quad \from * 0x200000 | PTTEF_PRESENT | PTTEF_PAGE_SIZE | PTTEF_US /* 0x200000 = 2MB */
		.if     \to-\from
			mapro "(\from+1)",\to		/* recursive function */
		.endif
	.endm

	/* Page tables (See AMD64 vol. 2, pp. 123, 4-Mbyte Page Translation) */
	/* Initial Page-Map Level-4 (PML4) table */
	.align	PAGE_SIZE
vmm_pml4_table:
	.quad	(PHYS(vmm_pdp_table) + PTTEF_PRESENT + PTTEF_RW + PTTEF_US) /* 0-th entry */
	.fill	261, 8, 0
	.quad	(PHYS(vmm_pdp_table) + PTTEF_PRESENT + PTTEF_RW + PTTEF_US) /* x-th entry where x = 262 = PML4 offset (39-47 bits) of VMM_OFFSET */
	.fill	249, 8, 0

	/* Initial Page-Directory-Pointer (PDP) table */
	.align	PAGE_SIZE
vmm_pdp_table:
	.quad	(PHYS(vmm_pdir_table) + PTTEF_PRESENT + PTTEF_RW + PTTEF_US)
	.fill	511, 8, 0

	/* Initial Page-Directory (PD) table */
	.align	PAGE_SIZE
vmm_pdir_table:
	/* Create identical map for pages that MAVMM need access to
	 * Access outside this region will create page fault 
	 */
	
	/* need first page to access device mem map (serial port, video buffer...)
	 * 16MB VMM memory region, from 0x8000000 to 0x8FFFFFF
	 */

	/* identmap 0, 0x3F */
	identmap 0, 0x3F
	identmap 0x40, 0x7F

/************************************ Others *******************************************************/

/* 4-byte region for multiboot info structure */
multiboot_ptr:
	.long 0x00000000

	.align	8
	.code64
boot_stack:
 	.fill	STACK_SIZE
