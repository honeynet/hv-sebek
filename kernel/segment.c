/*========================================================
**University of Illinois/NCSA
**Open Source License
**
**Copyright (C) 2007-2008,The Board of Trustees of the University of
**Illinois. All rights reserved.
**
**Developed by:
**
**    Research Group of Professor Sam King in the Department of Computer
**    Science The University of Illinois at Urbana-Champaign
**    http://www.cs.uiuc.edu/homes/kingst/Research.html
**
**Permission is hereby granted, free of charge, to any person obtaining a
**copy of this software and associated documentation files (the
**¡°Software¡±), to deal with the Software without restriction, including
**without limitation the rights to use, copy, modify, merge, publish,
**distribute, sublicense, and/or sell copies of the Software, and to
**permit persons to whom the Software is furnished to do so, subject to
**the following conditions:
**
*** Redistributions of source code must retain the above copyright notice,
**this list of conditions and the following disclaimers.
*** Redistributions in binary form must reproduce the above copyright
**notice, this list of conditions and the following disclaimers in the
**documentation and/or other materials provided with the distribution.
*** Neither the names of <Name of Development Group, Name of Institution>,
**nor the names of its contributors may be used to endorse or promote
**products derived from this Software without specific prior written
**permission.
**
**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
**EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
**MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
**IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
**ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
**TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
**SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
**==========================================================
*/

#include "types.h"
#include "vm.h"
#include "page.h"
#include "failure.h"

// return linear address from logical segment:offset address
// Note: if seg_select = 0 & protected mode => assume guest is using flat memory model
unsigned long glogic_2_glinear(struct vm_info * vm, u16 seg_select, u64 offset)
{
	//first checking if guest os is in real mode or protected mode
	//in real mode memory management mode is always segmentation, while
	//in protected mode, an examination of the bit PG of the register R0
	//should be conducted to know whether paging is enabled.
	//for more details, referencing to chapter 2 of "understanding the linux
	//kernel" and chapter 4 & 5 of "AMD vol 2"

	u8 pe = (vm->vmcb->cr0 & X86_CR0_PE) > 0;	// protected mode enable?
	u64 guest_linear;

	if (pe == 0)//real mode
	{
//		outf("Guest is in real mode\n");
		u16 MASK = (1 << 16) - 1;
		unsigned int realmode_offset = offset & MASK;
		guest_linear = seg_select * 0x10 + realmode_offset;
	}
	else {
//		outf("Guest is in protected mode\n");

		//assuming the guest is using flat memory model
		if (seg_select == 0) return offset;

		//extracting the bit PG of CR0 to know whether guest os using
		//paging or only segmentation
		u8 pg = (vm->vmcb->cr0 & X86_CR0_PG) > 0;
		u8 pse = (vm->vmcb->cr4 & X86_CR4_PSE) > 0;

		if (vm->vmcb->cr4 & X86_CR4_PAE) {
			fatal_failure("Converting address in 2mb PAE paging mode not supported yet!");
		}

		//TODO: give warning when selector points to LDT instead of GDT

		u64 gdtbase = vm->vmcb->gdtr.base;
		unsigned int gdt_index = seg_select >> 3; //13 most significant bits = index into the gdt

		//get pointer to the corresponding gdt entry
		u64 gdtentry_linearaddr = gdtbase + 8 * gdt_index;
		u64 gdtentry_physaddr;

//		outf("+++ GDTentry linear: %x\n", gdtentry_linearaddr);

		if(pg == 0) gdtentry_physaddr = gdtentry_linearaddr;
		else
		{
			if(pse == 0)//4kb
				gdtentry_physaddr = linear2physical_legacy4kb(vm->vmcb->cr3, gdtentry_linearaddr);
			else //4mb
				gdtentry_physaddr = linear2physical_legacy4mb(vm->vmcb->cr3, gdtentry_linearaddr);
		}

		// get the descriptor
		// gdtentry_physaddr is guest physical = host physical address of the gdt entry
		u64 seg_descriptor = * ((u64 *) gdtentry_physaddr);

		//bit 0->15 of the "BASE" field of segment descriptor, corresponding
		//bit 16 -> 31 of segment descriptor (page 38 understanding linux
		u16 base1;
		unsigned int temp = seg_descriptor >> 16;
		u16 MASK = (1 << 16) -1;
		base1 = temp & MASK;

		//bit 16->23 of the "BASE" field of segment descriptor, corresponding
		//bit 32 -> 39 of segment descriptor
		u8  base2;
		temp = seg_descriptor >> 32;
		MASK = (1 << 8) -1;
		base2 = temp & MASK;

		//bit 24->31 of the "BASE" field of segment descriptor, corresponding
		//bit 56->63 of segment descriptor
		u8 base3;
		temp = seg_descriptor >> 56;
		MASK = (1 << 8) - 1;
		base3 = temp & MASK;

		//get base address of the segment into seg_baseaddr
		u32 seg_baseaddr = base1 + (base2 << 16) + (base3 << 24);

		guest_linear = seg_baseaddr + offset;
//		outf("+++ guest linear: %x\n", guest_linear);
	}

	return guest_linear;
}
