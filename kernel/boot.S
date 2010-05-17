/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#define __ASSEMBLY__

#include "multiboot.h"
#include "msr.h"
#include "page.h"
#include "system.h"
#include "vmm.h"

#define CPU_ID_EXT_FEATURES	0

.section .text
.code32

.globl	start, _start

start:
_start:
	#mov		0xffffff, %dl
	jmp	physical_entry

	/*Anh - breakpoint: mov	0xffffff, %dl - set in Simnow using "BM 0xffffff", read chapter 10, Simnow Manual */
breakpoint:
	push 	%dx;
	mov		0xffffff, %dl
	pop 	%dx;
	ret;

/*
	Multiboot header. To checked by GRUB loader
	The Multiboot header must appear within the first 8192 bytes of the OS image,
	and must be longword (32-bit) aligned. In general, it should come as early as possible

	check http://www.gnu.org/software/grub/manual/multiboot/html_node/Header-layout.html#Header-layout
	and http://www.gnu.org/software/grub/manual/multiboot/html_node/OS-image-format.html#OS-image-format
*/

	.align	4		/* Align 32 bits boundary.  */
multiboot_header:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) /* checksum */

die:
	ud2

physical_entry:
	cld		// clear direction flag - x86 opcode ref: http://pdos.csail.mit.edu/6.828/2008/readings/i386/c17.htm
	cli		// clear interrupt flag - mask external interrupts.

			// When should re re-enable?
			// check multiboot specification for machine state after loader:
			// http://www.gnu.org/software/grub/manual/multiboot/html_node/Machine-state.html#Machine-state
			// The OS image must leave interrupts disabled until it sets up its own IDT

	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC, %eax		//eax should be 0x2BADB002 after receiving control from GRUB
	jne	die //Otherwise => some error

	// ebx contains pointer to multibook structure, describing machine configurations, command line, etc...
    // We save the multiboot info structure for later use.
    // check http://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html
    movl	%ebx, multiboot_ptr

	/************************** Set up DATA Descriptors.**************************/
 	lgdt 	boot_gdt_descr		// Initial value for GDTR, point to a handcrafted GDT
	movl 	$VMM_DS32, %eax		// All data segment registers point to the entry 2 of GDT
	movl 	%eax, %ds			// (read format of segment registers in AMD manual vol2)
	movl 	%eax, %es
	movl 	%eax, %ss

	// reinitialize CS register to $VMM_CS32
	ljmp	$(VMM_CS32), $virtual_entry
virtual_entry:

	/* Set up FPU.  */
	fninit

	/************ Enabling Paging...  **********************************************/

	/* ENABLING PSE (4MB pages instead of 4KB) by setting its flag in control register 4 - CR4.  */
	movl	$X86_CR4_PSE, %ecx				// read AMD manual vol2 for structure of CR4
	movl	%ecx, %cr4						// 2 mov because x86 does not allow move constants to CRs directly

	/* Load pagetable base register.  */
	movl	$vmm_page_dir, %eax	// CR3 contains physical address of page directory
	movl	%eax, %cr3				// (AMD vol2 page 120)

	/******************* Set up EFER (Extended Feature Enable Register).  *********************/
	movl	$MSR_EFER, %ecx		//set ecx = indentifier of EFER MSR (machine specific register)
	rdmsr						//read value of EFER MSR to eax

	//Enable system call extension (no SYSCALL, SYSRET)
	btsl	$_EFER_SCE, %eax	//Set bit 0

	movl	$CPU_ID_EXT_FEATURES, %edi	//CPU_ID_EXT_FEATURES = 0
	btl		$20, %edi        	/* CPUID 0x80000001, EDX[20] */
	jnc		1f
	btsl    $_EFER_NX, %eax		//Anh - enable no execute page-protection feature
1:      wrmsr					//write eax to EFER MSR


	/***************** Set initial value for CR0 (See AMD64 manual vol. 2, p. 42).  ******************/
	//Anh - removed WP - read-only page are not protected from supervisor-level writes
	movl	$(X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_AM | X86_CR0_PG), %eax
	//movl	$(X86_CR0_PE | X86_CR0_MP | X86_CR0_ET | X86_CR0_NE | X86_CR0_WP | X86_CR0_AM | X86_CR0_PG), %eax
	movl	%eax, %cr0

	/* Initialize the stack.  */
	movl	$(boot_stack) + STACK_SIZE, %esp
	movl    %esp, %ebp

	/* Reset EFLAGS (including interupt and direction flags). */
	pushl   $0
	popfl		//now eflag = 0

	/* Pass off the Multiboot info structure to C land (move its address into %edi). */
	pushl	multiboot_ptr;

	//passing of multiboot_ptr argument checked - OK
	// jump too start_vmm function, which is defined in setup.c
	call    start_vmm

	/* Force a panic (the program should not reach this point of the code).  */
	ud2

/********************************************************************************************/
/************************************ DATA **************************************************/
/********************************************************************************************/

.section .data
// Note: .hword = 16 bits (2 bytes), .long = 32 bits (4 bytes), .quad = 64 bits (8 bytes)

/************************************ Descriptors **************************************************/
	/* Initial selector for GDTR - AMD vol2 p72*/
	.align	4
boot_gdt_descr:
	.hword	GDT_ENTRIES * 8 - 1  /* limit */
	.long	gdt_table  /* address of GDT */

	/* Initial Global Descriptor Table (See AMD64 vol. 2 p78) */
	.align	PAGE_SIZE
gdt_table:
    .quad	0x0000000000000000     	/* null selector */
    .quad	0x00cf9e000000ffff     	/* ring 0 (dpl = 0), code, compatibility
    								TODO - Conforming = 1 or 0? vol2 p80
			          				(base = 0, limit = fffff, G (limit x 4KB),R, P, D (32 bits operands) */
			          				//11001111 [P=1][DPL=00][S=1][T=1110] 00000000 00000000 00000000 11111111 11111111
    .quad	0x00cf92000000ffff     	/* ring 0, data                */
    								//11001111 [P=1][DPL=00][S=1][T=0010] 00000000 00000000 00000000 11111111 11111111
    .quad	0x00cffe000000ffff     	/* ring 3 code, compatibility */
    								//11001111 [P=1][DPL=11][S=1][T=1110] 00000000 00000000 00000000 11111111 11111111
    .quad	0x00cff2000000ffff     	/* ring 3 data */
    								//11001111 [P=1][DPL=11][S=1][T=0010] 00000000 00000000 00000000 11111111 11111111
    .fill	7, 8, 0					/* space for TSS and LDT per CPU     */


/************************************ Paging *******************************************************/
	//check http://sourceware.org/binutils/docs-2.18/as/Macro.html#Macro
    .macro  identmap from, to
		.long \from * 0x400000 | PTTEF_PRESENT | PTTEF_RW | PTTEF_PAGE_SIZE | PTTEF_US	// 0x400000 = 4MB
		.if     \to-\from
			identmap "(\from+1)",\to		//recursive function
		.endif
	.endm

	//identical map, but read only
	.macro  mapro from, to
		.long \from * 0x400000 | PTTEF_PRESENT | PTTEF_PAGE_SIZE | PTTEF_US // 0x400000 = 4MB
		.if     \to-\from
			mapro "(\from+1)",\to		//recursive function
		.endif
	.endm

/* Page tables (See AMD64 vol. 2, pp. 123, 4-Mbyte Page Translation) */
/* Initial Page-Directory (PD) */
	.align	PAGE_SIZE
vmm_page_dir:
	/* Create identical map for pages that MAVMM need access to
	Access outside this region will create page fault */
	/*identmap 0, 0				//need first page to access device mem map (serial port, video buffer...)
	mapro 0x01, 0x1F
    identmap 0x20, 0x23			//16MB VMM memory region, from 0x8000000 to 0x8FFFFFF
    mapro 0x23, 0x3F*/

	identmap 0, 0x3F

/************************************ Others *******************************************************/

/* 4-byte region for multiboot info structure */
multiboot_ptr:
	.long 0x00000000

	.align	4
boot_stack:
 	.fill	STACK_SIZE
