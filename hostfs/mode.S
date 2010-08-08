/*
 * mode.S
 *
 *  Created on: May 22, 2009
 *      Author: anguyen7
 */

#define __ASSEMBLY__

#include "cpu.h"

#define SECTORS_PER_TRACK 	63
#define NUM_HEADS			2
#define SECTORS_PER_PLATE 	SECTORS_PER_TRACK * NUM_HEADS
#define DRIVE_NUMBER		0x80

.section .text
.code32

.global read_sector_asm

switch_2_pmode:
	push %eax
	#mov 0xffffff, %al
	mov %cr0, %eax
	orl $X86_CR0_PE, %eax
	orl $X86_CR0_PG, %eax
	mov %eax, %cr0
	pop %eax
	ret

switch_2_rmode:
	push %eax
	#mov 0xffffff, %al
	mov %cr0, %eax
	and $(0xFFFFFFFF - X86_CR0_PE), %eax
	and $(0xFFFFFFFF - X86_CR0_PG), %eax
	mov %eax, %cr0
	pop %eax
	ret

//ax contains dos sector number
lsector_to_psector:
	pushw %ax
	//physical_sector = 1 + (dos_sector  %  SECTORS_PER_TRACK)
	mov $SECTORS_PER_TRACK, %cl
	div %cl	//al contains the remainder of ax : cl
	add $1, %al
	// store physical_sector from al
	mov %al, physical_sector

	//physical_head = (dos_sector / SECTORS_PER_TRACK)  MOD  NUM_HEADS
	popw %ax
	pushw %ax
	mov $SECTORS_PER_TRACK, %cl
	div %cl
	mov %ah, %al
	xor %ah, %ah
	mov $NUM_HEADS, %cl
	div %cl
	// store physical_head from al
	mov %al, physical_head

	popw %ax
	//u8 physical_track = dos_sector / (SECTORS_PER_TRACK * NUM_HEADS)
	mov $SECTORS_PER_PLATE, %cl
	div %cl
	// store physical_track from ah
	mov %ah, physical_track

	ret

//eax contains sector number
//ebx contains return address
read_sector_asm:
	mov 0xffffff, %bl
	call lsector_to_psector
	call switch_2_rmode

	mov $0, %ax
	mov %ax, %es
	movl $tmp_buffer, %ebx	//return address

	mov $2, %ah	//read function
	mov $1, %al	//number of sector

	mov physical_sector, %cl
	mov physical_head, %dh
	mov physical_track, %ch

	mov $DRIVE_NUMBER, %dl

	int $0x13

	call switch_2_pmode

	ret

.data

tmp_buffer:
	.fill	512
physical_sector:
	.byte 0
physical_head:
	.byte 0
physical_track:
	.byte 0
