#ifndef __VM_H__
#define __VM_H__

#include "multiboot.h"
#include "vmcb.h"
#include "e820.h"
#include "page.h"

#define	GUEST_PADDR_MBI	0x2d0e0UL

#define VM_MAX_CUR_OPENFILE		(PAGE_SIZE_4KB / sizeof (fid2name_map))
#define VM_MAX_FILENAME 			20

#define VM_MAX_PNAME_LEN			20
#define VM_MAX_PROCESS_TRACKED		(PAGE_SIZE_4KB / MAX_PNAME_LEN)

//HeeDong - variables to store and load registers of Guest
// we don't need to store eax because eax is stored inside vmcb.rax
// and it will be loaded / stored with VMLOAD / VMSAVE (remember to use these instructions)
u64 g_rbp;
u64 g_rbx;
u64 g_rcx;
u64 g_rdx;
u64 g_rsi;
u64 g_rdi;
u64 g_r8;
u64 g_r9;
u64 g_r10;
u64 g_r11;
u64 g_r12;
u64 g_r13;
u64 g_r14;
u64 g_r15;

#ifndef __ASSEMBLY__

struct fid2name_map {
	int fid;
	char fname[VM_MAX_FILENAME];
};

struct syscall_info {
	int syscallno;
	unsigned long arg;
	int datasize;
	int socketno; /* used in case syscallno = sys_socketcall, specifying
		           * whether it's an sys_recv, sys_recvfrom,... */
};

//map from thread id to syscall number that the thread is waiting on
struct tid2syscall_map {
	int tid;
	struct syscall_info info;
};

struct vm_info
{
	struct vmcb *vmcb;
	struct vmcs *vmcs;

	unsigned int asid;
	unsigned long n_cr3;  /* [Note] When #VMEXIT occurs with
						   * nested paging enabled, hCR3 is not
						   * saved back into the VMCB (vol2 p. 409)???*/
	
	unsigned long io_intercept_table;

	unsigned long msr_intercept_table;

	/* Intel VMX only */
	unsigned int			msr_count;
	struct vmx_msr_entry	*msr_area;
	
	unsigned int			host_msr_count;
	struct vmx_msr_entry	*host_msr_area;

	char waitingRetSysCall;

	u64 org_sysenter_cs;	//original sysenter msrs, used when syscall interception is enabled

	int itc_flag; 			//flags specifying which interceptions were
							//registered for this vm (see user.h)
	int itc_skip_flag;

	//mapping from id of files opened by this VM to their names
	struct fid2name_map * fmap;
	int nOpenFile;

	struct tid2syscall_map * syscallmap;
	int nWaitingThreads;

	//list of process names to be tracked
	int nTrackedProcess;
	char * ptracked;
	u8 btrackcurrent;	//whether the current process is being tracked or not
	
	//on & exit handler
	void (*vm_run) (struct vm_info * vm);
	void (*vm_exit) (struct vm_info * vm);
	
	//event handler pointer
	void (*handle_wrmsr) (struct vm_info * vm);
	void (*handle_exception) (struct vm_info * vm);
	void (*handle_swint) (struct vm_info * vm);
	void (*handle_npf) (struct vm_info * vm);
	void (*handle_vmcall) (struct vm_info * vm);
	void (*handle_iret) (struct vm_info * vm);
	void (*handle_cr3_write) (struct vm_info * vm);
	void (*handle_popf) (struct vm_info * vm);
};

extern unsigned long create_intercept_table ( unsigned long size );

extern void vm_enable_intercept(struct vm_info * vm, int flags);
extern void vm_disable_intercept(struct vm_info *vm, int flags);

extern int vm_is_process_tracked(struct vm_info *vm, char * pname);
extern void vm_remove_tracked_process(struct vm_info *vm, char * pname);
extern void vm_add_tracked_process(struct vm_info *vm, char * pname);

extern void vm_add_waiting_thread(struct vm_info *vm, int tid, struct syscall_info * info);
extern void vm_remove_waiting_thread(struct vm_info *vm, int tid);
extern struct syscall_info * vm_get_waiting_syscall(struct vm_info *vm, int tid);

extern void vm_add_fileid2name_map(struct vm_info * vm, int id, char * name);
extern void vm_remove_fileid2name_map(struct vm_info * vm, int id);
extern char * vm_get_fname_from_id(struct vm_info * vm, int id);

extern void vm_create ( struct vm_info *vm, unsigned long vmm_pmem_start,
		unsigned long vmm_pmem_size, struct e820_map *e820);
extern void vm_init (struct vm_info *vm);
extern void vm_boot (struct vm_info *vm);
extern void print_page_errorcode(u64 errcode);

#endif

#endif /* __VM_H__ */
