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

struct fid2name_map {
	int fid;
	char fname[VM_MAX_FILENAME];
};

struct syscall_info {
	int syscallno;
	int arg;
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

	//TODO: Anh - check if this is necessary or not.
	// seem that the guest can't modify vmcb->n_cr3
	// so there is no need to save it
	unsigned long n_cr3;  /* [Note] When #VMEXIT occurs with
						   * nested paging enabled, hCR3 is not
						   * saved back into the VMCB (vol2 p. 409)???*/

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
};

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
extern void vm_boot (struct vm_info *vm);
extern void print_page_errorcode(u64 errcode);

#endif /* __VM_H__ */
