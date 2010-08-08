/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#define __ASSEMBLY__

#define VMRUN  .byte 0x0F,0x01,0xD8
#define VMLOAD .byte 0x0F,0x01,0xDA
#define VMSAVE .byte 0x0F,0x01,0xDB
#define STGI   .byte 0x0F,0x01,0xDC
#define CLGI   .byte 0x0F,0x01,0xDD

.section .text
.code64

	.global svm_launch
svm_launch:
	CLGI

    /* save host state */
	push %rbx
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
	/*
	%cr2
	%fs
	%gs
	ldt
	msr_gs_base
	*/

	//breakpoint
	push %dx
	mov 0xffffff, %dl
	pop %dx

	/* restore guest state */
	call restore_guest_states
	VMLOAD

	VMRUN

	//breakpoint
	//push %dx
	//mov 0xffffff, %dl
	//pop %dx

	/* save guest state */
	VMSAVE
	call save_guest_states

	/* restore host state */
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rbx

	STGI

	ret

restore_guest_states:
	pushw %dx
	mov first_boot, %dx
	cmp $1, %dx
	je first

	popw %dx
	movq g_rbx, %rbx	//no need to restore eax, since it will be restored from vmcb
	movq g_rcx, %rcx
	movq g_rdx, %rdx
	movq g_rsi, %rsi
	movq g_rdi, %rdi
	movq g_rbp, %rbp
	movq g_r8, %r8
	movq g_r9, %r9
	movq g_r10, %r10
	movq g_r11, %r11
	movq g_r12, %r12
	movq g_r13, %r13
	movq g_r14, %r14
	movq g_r15, %r15
	ret

first:	//initialize some guest states when we move into it for the first time
	popw %dx
	movw $0, first_boot
	movq $0, %rbx
	movq $0, %rcx
	movq $0x80, %rdx
	movq $0, %rsi
	movq $0, %rdi
	movq $0, %rbp
	movq $0, %r8
	movq $0, %r9
	movq $0, %r10
	movq $0, %r11
	movq $0, %r12
	movq $0, %r13
	movq $0, %r14
	movq $0, %r15
	ret

save_guest_states:
	movq %rbx, g_rbx //no need to save eax, since it is already saved in vmcb
	movq %rcx, g_rcx
	movq %rdx, g_rdx
	movq %rsi, g_rsi
	movq %rdi, g_rdi
	movq %rbp, g_rbp
	movq %r8, g_r8
	movq %r9, g_r9
	movq %r10, g_r10
	movq %r11, g_r11
	movq %r12, g_r12
	movq %r13, g_r13
	movq %r14, g_r14
	movq %r15, g_r15
	ret

first_boot: .word 1	//.word => 2 bytes = sizeof(dx)
#	ud2a

