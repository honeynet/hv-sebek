/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#define __ASSEMBLY__

#define VMRUN  .byte 0x0F,0x01,0xD8
#define VMLOAD .byte 0x0F,0x01,0xDA
#define VMSAVE .byte 0x0F,0x01,0xDB
#define STGI   .byte 0x0F,0x01,0xDC
#define CLGI   .byte 0x0F,0x01,0xDD

	.global svm_launch
svm_launch:
	CLGI

    /* save host state */
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
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
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx

	STGI

	ret

restore_guest_states:
	pushw %dx
	mov first_boot, %dx
	cmp $1, %dx
	je first

	popw %dx
	movl g_ebx, %ebx	//no need to restore eax, since it will be restored from vmcb
	movl g_ecx, %ecx
	movl g_edx, %edx
	movl g_esi, %esi
	movl g_edi, %edi
	movl g_ebp, %ebp
	ret

first:	//initialize some guest states when we move into it for the first time
	popw %dx
	movw $0, first_boot
	movl $0, %ebx
	movl $0, %ecx
	movl $0x80, %edx
	movl $0, %esi
	movl $0, %edi
	movl $0, %ebp
	ret

save_guest_states:
	movl %ebx, g_ebx //no need to save eax, since it is already saved in vmcb
	movl %ecx, g_ecx
	movl %edx, g_edx
	movl %esi, g_esi
	movl %edi, g_edi
	movl %ebp, g_ebp
	ret

first_boot: .word 1	//.word => 2 bytes = sizeof(dx)
#	ud2a

