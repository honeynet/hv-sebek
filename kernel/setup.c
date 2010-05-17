/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "types.h"
#include "failure.h"
#include "string.h"
#include "page.h"
#include "multiboot.h"
#include "e820.h"
#include "pmem_layout.h"
#include "alloc.h"
#include "cpu.h"
#include "vm.h"
#include "vmm.h"
#include "serial.h"

struct cmdline_option
{
	unsigned long vmm_pmem_size;
};

static struct cmdline_option __init parse_cmdline ( const struct multiboot_info *mbi )
{
	/*TODO: get VMM pmem size and VM pmem size from cmd line
		for now just return the default value, regardless of the content of cmd line
	*/

	struct cmdline_option opt = { DEFAULT_VMM_PMEM_SIZE };

	// Print the command line
	if ( ( mbi->flags & MBI_CMDLINE ) && ( mbi->cmdline != 0 ) )
	{
		char *cmdline = (char*)(unsigned long)mbi->cmdline;
		outf ("Command line passed to MAVMM: %s\n", cmdline );
	}

	return opt;
}

static void __init setup_memory(const struct multiboot_info *mbi,
		const struct cmdline_option *opt, struct pmem_layout *pml )
{
	//get the map of machine memory regions from mbi (provided by GRUB)
	//each mem region will have a start address x, size s, and a type t (usable, reserved, ...)
	//store the map to pml->e820
	setup_memory_region (&(pml->e820), mbi);

	// get the number of memory pages, and number of the LAST page usable as RAM (has E820_RAM type)
	pml->total_pages  = get_nr_pages ( &(pml->e820) );
	pml->max_page     = get_max_pfn ( &(pml->e820) );

	// VMM starts at 0x800000 = 128MB
	pml->vmm_pmem_start = DEFAULT_VMM_PMEM_START;
	// VMM heap at the end of VMM memory region
	pml->vmm_pmem_end = pml->vmm_pmem_start + opt->vmm_pmem_size - 1;

	// Allocate and initialize VMM memory allocation bitmap
	naive_memmap_init ( &(pml->e820), pml );
}

void print_pmem_layout(struct pmem_layout * pml)
{
	outf("Physical memory layout:\n");
	outf("VMM pmem start: %x\n", pml->vmm_pmem_start);
	outf("VMM heap start: %x\n", pml->vmm_heap_start);
	outf("VMM pmem end: %x\n", pml->vmm_pmem_end);
	outf("Max page: %x\n", pml->max_page);
	outf("Total pages: %x\n", pml->total_pages);
}

void __init start_vmm ( const struct multiboot_info *mbi )
{
	//Initialize serial port COM1, this must be done before calling outf
	setup_serial();
	outf("\n\n\n!!!!!!!!!!!BEGIN!!!!!!!!!!!\n\n\n");

	//Parse the command line that user pass to GRUB
	struct cmdline_option opt = parse_cmdline ( mbi );

	//Set up memory layout and store the layout in pml
	struct pmem_layout pml;
	setup_memory(mbi, &opt, &pml);

	outf("\n++++++ Enable SVM feature on CPU\n");
	enable_amd_svm();

	struct vm_info vm;
	vm_create (&vm, pml.vmm_pmem_start, opt.vmm_pmem_size, &(pml.e820));

	//Debug
	//e820_print_map(&(pml.e820));

	outf ("\n++++++ New virtual machine created. Going to GRUB for the 2nd time\n");
	vm_boot (&vm);
}
