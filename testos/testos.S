#define __ASSEMBLY__

#include "multiboot.h"

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002

.section .text
.code32

.globl	start, _start

start:
_start:

	mov	0xfffff, %dl
	int $0x80
	jmp die

	.align	4		/* Align 32 bits boundary.  */
multiboot_header:
	.long	MULTIBOOT_HEADER_MAGIC
	.long	MULTIBOOT_HEADER_FLAGS
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) /* checksum */

die:
	ud2
/********************************************************************************************/
/************************************ DATA **************************************************/
/********************************************************************************************/

.section .data

