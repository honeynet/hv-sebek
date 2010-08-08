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

#include "user.h"
#include "intercept.h"
#include "serial.h"
#include "svm.h"
#include "vm.h"
#include "failure.h"
#include "string.h"
#include "page.h"
#include "segment.h"
#include "msr.h"


/*****************************************************************************************/
/************************ MISCELANEOUS FUNCTIONS ***************************************/
/*****************************************************************************************/

// Assumption: GP = HP
unsigned long glogic_2_hphysic(struct vm_info * vm, u16 seg_select, u64 offset)
{
	u64 glinear = glogic_2_glinear(vm, seg_select, offset);
	u64 gphysic = linear_2_physical(vm->vmcb->cr0, vm->vmcb->cr3, vm->vmcb->cr4, glinear);

	return gphysic;
}

/***************************************************/

//this function handles getting  task struct's address of current process
//the purpose is to enable VMM to intercept processes with given names
//Given names come from users in the user mode of the guest os
unsigned long get_cur_process_taskstruct(struct vm_info * vm)
{
	//note: this function only works if the current  process has entered
	//the kernel mode. In kernel mode, each process's granted an 8kb-sized memory area
	//which contains a stack starts at the beginning of the area (with highest address)
	//and a struct named "thread_info" start at the end. Since the area
	//is a contiguous one, an operation: esp & 0xffffe000  helps bring out
	//the address of "thread_info". Since kernel only has one data segment
	//and one stack segment and kernel set their base address to 0, the address attaining from the above operation
	//is the linear one. Once having the linear address of thread_info, the task_struct's address
	//can be easily calculated from the first field of "thread_info"; again, for the
	//same reason this is a linear one.

	//linux can have stack size of 8kb or 4kb, in this case an 8kb stack size
	//is assumed to be used. in case of 4kb stack, the operation should be
	//esp & 0xfffff000

	//TODO: if guest is int usermode => do nothing, let guest OS handle

	unsigned long thread_info_gl = vm->vmcb->rsp & 0xffffe000;
	unsigned long thread_info_hp = glogic_2_hphysic(vm, vm->vmcb->ss.sel, thread_info_gl);

	//get the linear address of task_struct which is the first 4 bytes
	//in thread_info
	unsigned long task_struct_gl = *((unsigned long *) thread_info_hp);
	unsigned long task_struct_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, task_struct_gl);
	if(task_struct_hp == 0)
		outf(" task_struct_gl = %x, ds = %x, ss = %x", task_struct_gl, vm->vmcb->ds.sel,vm->vmcb->ss.sel);

	return task_struct_hp;
}

//get address of task_struct when process is in usermode
//in this case, task register (tr) is   resorted
unsigned long get_cur_process_usermode_taskstruct(struct vm_info * vm)
{
	//in task state segment, stack segment & stack offset of kernel stack
	//are stored in ss0 & esp0. for more information, reading chapter
	//12 in AMD vol.2

	unsigned long esp_hp = glogic_2_hphysic(vm, vm->vmcb->tr.sel, 0x04);
	if(esp_hp == -1) return -1;
	unsigned long esp = *((unsigned long*)esp_hp);
//	outf("esp: %x\n", esp);

	//get kernel stack segment address, 8th offset in tss
	unsigned long ss_hp  = glogic_2_hphysic(vm, vm->vmcb->tr.sel, 0x08);
	if(ss_hp == -1) return -1;
	//calculate stack segment
	u16 ss = *((u16 *)ss_hp);
//	outf("ss: %x\n", ss);

	unsigned long thread_info_gl = esp & 0xffffe000;
	unsigned long thread_info_hp = glogic_2_hphysic(vm, ss, thread_info_gl);
	if(thread_info_hp == -1)return -1;
	//get address of task_struct which is stored as first field of
	//thread_info
//	outf("thread_info_hp: %x\n", thread_info_hp);

	unsigned long task_struct_gl = *((unsigned long *) thread_info_hp);
	unsigned long task_struct_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, task_struct_gl);
//	outf("task_struct_hp: %x\n", task_struct_hp);

	return task_struct_hp;
}

//after having the host physical address of task_struct, current process's name
//can be deduced by adding the task_struct's physical address with the
//variable "comm"'s offset which is 0x18C in Linux 2.6
//after having the host physical address of task_struct, current process's name
//can be deduced by adding the task_struct's physical address with the
//variable "comm"'s offset which is 0x18C in Linux 2.6
char * get_cur_process_name(struct vm_info * vm, int mode)
{
	unsigned long task_struct_hp;

	if (mode == 0)//kernel mode
	task_struct_hp = get_cur_process_taskstruct(vm);
	else//user mode
	task_struct_hp = get_cur_process_usermode_taskstruct(vm);
	char * pname = (char*)(task_struct_hp + 0x18C);
	if(task_struct_hp != -1)
	{
		outf("+++++ process name: %s\n", pname);
//		outf("+++++ task_struct_hp: %x\n", task_struct_hp);
	}

	return pname;
}

//for get process's id and thread's id,  each process id and thread id specify an unique
//thread.
int get_cur_threadid(struct vm_info * vm, int mode)
{
	unsigned long task_struct_hp;

	if (mode == 0)	task_struct_hp = get_cur_process_taskstruct(vm); //kernel mode
	else task_struct_hp = get_cur_process_usermode_taskstruct(vm); //user mode

//	int pid = *((int *)(task_struct_hp + 0xA6));
	int tid = *((int *)(task_struct_hp + 0xA8));
	if(task_struct_hp != -1)
	{
//		outf("+++++ process id: %x\n", pid);
//		outf("+++++ thread id: %x\n", tid);
		return tid;
	}

	return 0;
}


// Skip a specific intercept for current instruction, before returning control to the guest
// used to let the guest handle the intercepted instruction
void __skip_intercpt_cur_instr (struct vm_info * vm, int flag){
	vm_disable_intercept(vm, flag);
	vm->itc_skip_flag |= flag;
	vm_enable_intercept(vm, USER_SINGLE_STEPPING);
}

/*****************************************************************************************/
/************************ HANDLING SYSTEMCALL INTERCEPTION ******************************/

struct syscall_entry {
   int sys_nargs;
   int	sys_flags;
   const char * sys_name;
};

struct syscall_entry syscalls[]={
	#include "syscall_ent.h"
};

/***************************************************/

//for handling the system call which loads shared libraries.
//In this system call,
//libary's name is contain in a memory area referenced by ebx
void sys_uselib( struct vm_info *vm)
{
        char * library;
        unsigned long arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);

        if(arg1 != -1)
        {
			library = (char *) arg1;
			outf("library used: = %s",library);
        }
}

/***************************************************/

//for handling the system call opening files, in this system call
//file's name is contain in a memory area referenced by ebx
//flags in ecx and mode in edx
void sys_open( struct vm_info *vm)
{
	char * filename = (char *) glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);

	if((long) filename != -1)
	{
		const unsigned long MASK = ( ( 1 << 16 ) - 1 );
		int flags = g_rcx & MASK;
		int mode =  g_rdx & MASK;
		outf("filename: %s, flags: %x, mode: %x", filename, flags, mode);

		int tid = get_cur_threadid(vm, 3);

		struct syscall_info info;
		info.syscallno = SYS_OPEN;
		info.arg = (long) filename;	//hp of file name

		vm_add_waiting_thread(vm, tid, &info);
	}
}

void sys_open_iret(struct vm_info *vm, struct syscall_info * info)
{
	char * filename = (char *) info->arg;

	vm_add_fileid2name_map(vm, vm->vmcb->rax, filename);
}

/***************************************************/

// for handling close system call
void sys_close(struct vm_info *vm)
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x", g_rbx);	//arg 1
	else outf("filename: %s", fname);

}

/***************************************************/

// for handling waitpid syscall
void sys_waitpid( struct vm_info *vm )
{
	int arg1;
	int* arg2;
	unsigned long host_p_addr_arg2;
	int arg3;

	// refer to 'man 2 waitpid'
	arg1 = (int) g_rbx;
	if (arg1 < -1)
		outf("Waiting for any child process whose process group ID is %x, ", -arg1);
	else if(arg1 == -1)
		outf("Waiting for any child process, ");
	else if(arg1 == 0)
		outf("Waiting for child process whose process group ID is equal to that of the calling process, ");
	else if(arg1 > 0)
		outf("Waiting for child process whose process ID is %x, ", arg1);
	else
		outf("Error: Should not reach here");

	host_p_addr_arg2 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx);
	arg2 = (int *) host_p_addr_arg2;
	if (arg2 != NULL) outf("Status: %x, ", *arg2);
	else outf("Status: NULL, ");

	arg3 = (int) g_rdx;
	outf("Option: %x", arg3);

}

/***************************************************/
// make a new name for a file
void sys_link( struct vm_info *vm )
{
	int *arg1;
	int *arg2;
	unsigned long host_p_addr_arg1;
	unsigned long host_p_addr_arg2;

	host_p_addr_arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);
	arg1 = (int *) host_p_addr_arg1;
	if (arg1 != NULL) outf("Old Filename: %s, ", arg1);

	host_p_addr_arg2 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx);
	arg2 = (int *) host_p_addr_arg2;
	if (arg2 != NULL) outf("New Filename: %s", arg2);

}

/***************************************************/
// delete a name and possibly the file it refers to
void sys_unlink( struct vm_info *vm )
{
	int *arg1;
	unsigned long host_p_addr_arg1;

	host_p_addr_arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);
	arg1 = (int *) host_p_addr_arg1;
	if (arg1 != NULL) outf("Filename: %s", arg1);
}


/***************************************************/
// delete a name and possibly the file it refers to
void sys_mmap( struct vm_info *vm )
{
	unsigned int arg2;
	int arg3, arg4;
	unsigned int arg6;

	outf("Starting Addr: %x, ", g_rbx); //arg1

	arg2 = g_rcx;
	outf("Length: %x, ", arg2);

	arg3 = g_rdx;
	outf("MEM Protection: %x, ", arg3);

	arg4 = g_rsi;
	outf("Flag: %x, ", arg4);

	char * fname = vm_get_fname_from_id(vm, g_rdi);
	if (fname == NULL) outf("fd: %x, ", g_rdi);
	else outf("filename: %s, ", fname);

	arg6 = g_rbp;
	outf("Offset: %x", arg6);

}


/***************************************************/
// delete a name and possibly the file it refers to
void sys_munmap( struct vm_info *vm )
{
	unsigned int arg2;

	outf("Starting Addr: %x, ", g_rbx);

	arg2 = g_rcx;
	outf("Length: %x", arg2);

}

/***************************************************/
//  check real user’s permissions for a file
void sys_access( struct vm_info *vm )
{
	int *arg1;
	unsigned long host_p_addr_arg1;

	host_p_addr_arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);
	arg1 = (int *) host_p_addr_arg1;
	if (arg1 != NULL) outf("Filename to check: %s, ", arg1);

	int arg2;
	arg2 = g_rcx;

	switch(arg2)
	{
		case R_OK: outf("Mode: Readable"); break;
		case W_OK: outf("Mode: Writable"); break;
		case X_OK: outf("Mode: Executable"); break;
		case F_OK: outf("Mode: File exists"); break;
	}


}

/***************************************************/
//  change ownership of a file
void sys_lchown( struct vm_info *vm )
{
	int *arg1;
	unsigned long host_p_addr_arg1;

	host_p_addr_arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);
	arg1 = (int *) host_p_addr_arg1;
	if (arg1 != NULL) outf("Filename: %s, ", arg1);

	int arg2;
	arg2 = g_rcx;
	outf("Owner ID: %x, ", arg2);

	int arg3;
	arg3 = g_rdx;
	outf("Group ID: %x", arg3);

}

/***************************************************/
// reposition read/write file offset
void sys_lseek( struct vm_info *vm )
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x, ", g_rbx);	//arg 1
	else outf("filename: %s, ", fname);

	int arg2;
	arg2 = g_rcx;
	outf("Offset: %x, ", arg2);

	int arg3;
	arg3 = g_rdx;
	outf("whence: %x", arg3);

}

/***************************************************/

//for handling the system call reading content of files
//in fact, it is more precise to handling this system call
//after an "iret" event instead of "int 80" event because
//when an "iret" event gets raised it means VMM can get
//returned data from registers.
void sys_read( struct vm_info *vm)
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x, ", g_rbx);
	else outf("filename: %s, ", fname);	//arg 1

	outf("buffer: %x, ", g_rcx); //arg 2

	const unsigned long MASK = ( ( 1 << 16 ) - 1 );
	int bufsize = g_rdx & MASK; //arg 3
	outf("size: %x", bufsize);

	//get tid of current process & add it to list of waiting thread
	int tid = get_cur_threadid(vm, 3);
//	outf(", thread id: %x", tid);

	struct syscall_info info;
	info.syscallno = SYS_READ;
	info.arg = g_rcx;

	vm_add_waiting_thread(vm, tid, &info);

//	outf("file dumped in the hexa format:  ");
//
//	int i;
//	for(i = 0; i < bufsize; i++) outf("%x ", *(buf + i));
}

//for handling sys_read in an "iret" event
void sys_read_iret(struct vm_info *vm, struct syscall_info * info)
{
	int i;

	outf("sys_read return value: %x\n", vm->vmcb->rax);

	if(vm->vmcb->rax > 0)
	{
		unsigned long buffer_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, info->arg);

		if (buffer_hp != -1)
		{
			//print out first 10 bytes
			outf("first 10 bytes: ");

			int len = 10;
			if (len > vm->vmcb->rax) len = vm->vmcb->rax;
			for(i = 0; i < len; i++) outf( "%x ", *((char *)(buffer_hp + i)) & 0xFF );

			outf("\n");
		}
	}
}

/***************************************************/

//for handling the system call writing to files
//ecx contains address of buffered data, edx indicates data size
//ebx contains a file descriptor
void sys_write( struct vm_info *vm)
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x, ", g_rbx);	//arg 1
	else outf("filename: %s, ", fname);

	outf("size: %x, ", g_rdx);	//arg 3

	//host physical address of data
	unsigned long arg2 =  glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx);

	if(arg2 != -1) outf("content written [%s]", arg2);
}

/***************************************************/

// for handling ioctl system call
void sys_ioctl( struct vm_info *vm )
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x, ", g_rbx);	//arg 1
	else outf("filename: %s, ", fname);

	int arg2;
	char * arg3;
	unsigned long host_p_addr_arg3;

	arg2 = (int) g_rcx;
	outf("Device-dependent request code: %x, ", arg2);

	outf("Memory Addr: %x", g_rdx); //arg 2

}

/***************************************************/

/*haind: the following functions handle network system calls. One network system call often
 * starts by calling sys_socketcall(int call, unsigned long __user *args), then depends on
 * the value of call successive sytem calls would be resorted*/
//for handling data received from outside hosts, should be call from an "iret" event
void sys_recvfrom(struct vm_info *vm)
{
	 unsigned long buffer_gl = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + 4));
	 unsigned long peer_info_gl = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + 16));

	 const unsigned long MASK = ( ( 1 << 16 ) - 1 );
	 int bufsize = (*((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + 8))) & MASK;

	 unsigned char *peer_ip; //ip of peer
	 unsigned long peer_info_hp; //pointer to sockaddr_in corresponding to peer

	 peer_info_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, peer_info_gl);
	 peer_ip = (unsigned char*)&(((struct sockaddr_in*)peer_info_hp)->sin_addr);

	 outf("receving %x bytes from  %x.%x.%x.%x : %x  ", bufsize, peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], ((struct sockaddr_in*)peer_info_hp)->sin_port);


	 //get tid of current process & add it to list of waiting thread
	 int tid = get_cur_threadid(vm, 102);
	 struct syscall_info info;
	 info.syscallno = SYS_SOCKETCALL;
	 info.arg = buffer_gl;
	 info.socketno = 12; /*indicate that it's sys_recvfrom in sys_socketcall*/
	 info.datasize = bufsize;
	 vm_add_waiting_thread(vm, tid, &info);

}

/***************************************************/

void sys_recvfrom_iret(struct vm_info *vm, struct syscall_info * info)
{
	int i;

	unsigned long buffer_hp = glogic_2_hphysic(vm, 0, info->arg);

	if (buffer_hp != -1)
	{

		//outf("receving %x bytes from  %x.%x.%x.%x : %x  ", bufsize, peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], ((struct sockaddr_in*)peer_info_hp)->sin_port);

		for(i = 0; i < info->datasize; i++) outf( "%x ", *((char *)(buffer_hp + i)) & 0xFF );

		outf("\n");
	}


}


/***************************************************/

void sys_sendto(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
	 	 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 const unsigned long MASK = ( ( 1 << 16 ) - 1 );
	 int bufsize = arg[2] & MASK;
	 unsigned char *peer_ip; //ip of peer
	 unsigned long peer_info_hp; //pointer to sockaddr_in corresponding to peer

	 peer_info_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, arg[4]);
	 peer_ip = (unsigned char*)&(((struct sockaddr_in*)peer_info_hp)->sin_addr);


	 unsigned long buffer_hp = glogic_2_hphysic(vm, 0, arg[1]);

	 	 if (buffer_hp != -1)
	 	 	{

				outf("sending %x bytes to  %x.%x.%x.%x:%x ", bufsize, peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], ((struct sockaddr_in*)peer_info_hp)->sin_port);

	 	 		for(i = 0; i < bufsize; i ++) outf( "%x ", *((char *)(buffer_hp + i)) & 0xFF );

	 	 		outf("\n");
	 	 	}

}


/***************************************************/

void sys_socket(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
	 	 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 outf("sys_socket  family %x, type %x protocol %x", arg[0], arg[1], arg[2]);


}

/***************************************************/
void sys_bind(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
		 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 unsigned char *peer_ip; //ip of peer
	 unsigned long peer_info_hp; //pointer to sockaddr_in corresponding to peer

	 peer_info_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, arg[1]);
	 peer_ip = (unsigned char*)&(((struct sockaddr_in*)peer_info_hp)->sin_addr);

	 outf("sys_bind fd %x peer's ip %x.%x.%x.%x addrlen %x", arg[0], peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], arg[2]);

}
/***************************************************/
void sys_connect(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
		 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 unsigned char *peer_ip; //ip of peer
	 unsigned long peer_info_hp; //pointer to sockaddr_in corresponding to peer

	 peer_info_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, arg[1]);
	 peer_ip = (unsigned char*)&(((struct sockaddr_in*)peer_info_hp)->sin_addr);

	 outf("sys_connect fd %x peer's ip %x.%x.%x.%x addrlen %x", arg[0], peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3], arg[2]);
}
/***************************************************/
void sys_listen(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
		 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 outf("listen  fd %x, backlog %x", arg[0], arg[1]);
}
/***************************************************/


void sys_accept(struct vm_info *vm)
{
	 unsigned long arg[6]; // array of parameters in sys_recvfrom
	 int i;
	 for (i = 0; i < 6; i ++)
		 arg[i] = *((unsigned long *)glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx + i * 4));

	 unsigned char *peer_ip; //ip of peer
	 unsigned long peer_info_hp; //pointer to sockaddr_in corresponding to peer

	 peer_info_hp = glogic_2_hphysic(vm, vm->vmcb->ds.sel, arg[1]);
	 peer_ip = (unsigned char*)&(((struct sockaddr_in*)peer_info_hp)->sin_addr);

	 outf("sys_accept fd %x peer's ip %x.%x.%x.%x ", arg[0], peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3]);
}


/***************************************************/

void sys_socketcall(struct vm_info *vm)
{

	int call;

    call = (int) g_rbx;

	switch(call)
	{
	//case SYS_SEND: sys_send(vm); break;
	case SYS_SOCKET: sys_socket(vm); break;
	case SYS_BIND:  sys_bind(vm); break;
	case SYS_CONNECT: sys_connect(vm); break;
	case SYS_LISTEN: sys_listen(vm); break;
	case SYS_ACCEPT: sys_accept(vm); break;
	case SYS_SENDTO: sys_sendto(vm); break;

	case SYS_RECVFROM: sys_recvfrom(vm); break;

	default: return; break;
	}
}

/***************************************************/

void sys_execve( struct vm_info *vm )
{
	unsigned long host_p_addr_arg1;
	unsigned long host_p_addr_arg2;
	unsigned long host_p_addr_arg3;
	char *arg1;
	char **arg2;
	char **arg3;
	int i=0;
	unsigned long temp_addr;
	char *temp;

	host_p_addr_arg1 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rbx);
	arg1 = (char *) host_p_addr_arg1;
	if (host_p_addr_arg1 != -1) outf("Filename: %s ", arg1);

	host_p_addr_arg2 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rcx);
	if (host_p_addr_arg2 != -1) {
		arg2 = (char **) host_p_addr_arg2;

		outf("Arguments[");

		while(arg2[i] != NULL)
		{
			temp_addr = glogic_2_hphysic(vm, vm->vmcb->ds.sel, (u64) arg2[i]);
			temp = (char *) temp_addr;
			if (temp_addr != -1) outf("%s", temp);
			temp_addr = 0;
			temp = NULL;
			if(arg2[i+1] != NULL) outf(", ");
			i++;
		}
	}

	i = 0;
	outf("], key=value pairs[");

	host_p_addr_arg3 = glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rdx);
	if (host_p_addr_arg3 != -1) {
		arg3 = (char **) host_p_addr_arg3;
		while(arg3[i] != NULL)
		{
			temp_addr = glogic_2_hphysic(vm, vm->vmcb->ds.sel, (u64) arg3[i]);
			temp = (char *) temp_addr;
			if (temp != (char *)-1) outf("%s", temp);
			temp_addr = 0;
			temp = NULL;
			if(arg3[i+1] != NULL)
			{
				outf(", ");
			}
			i++;
		}
	}

	outf("]");
}

/***************************************************/

void sys_fstat( struct vm_info *vm)
{
	char * fname = vm_get_fname_from_id(vm, g_rbx);
	if (fname == NULL) outf("fd: %x, ", g_rbx);	//arg 1
	else outf("filename: %s, ", fname);

	outf("Addr to Stat Structure: %x", g_rcx); //arg 2

}

/***************************************************/

void __handle_syscall( struct vm_info *vm )
{
	unsigned int sysCallNo;
	unsigned int sys_nargs;
	int i;
	unsigned int arg;

	// Collect the system call number from eax register (eax is rax of VMCB)
	sysCallNo = (unsigned int) vm->vmcb->rax;
	outf(">> %s( ",syscalls[sysCallNo].sys_name);

	// Find out the number of arguments to the system call
	sys_nargs = syscalls[sysCallNo].sys_nargs;

	switch(sysCallNo)
	{
		case SYS_READ: sys_read(vm); break;
		case SYS_WRITE:	sys_write(vm); break;
		case SYS_OPEN:	sys_open(vm); break;
		case SYS_CLOSE:	sys_close(vm); break;
		case SYS_WAITPID: sys_waitpid(vm); break;
		case SYS_LINK: sys_link(vm); break;
		case SYS_UNLINK: sys_unlink(vm); break;
		case SYS_EXECVE: sys_execve(vm); break;
		case SYS_LCHOWN: sys_lchown(vm); break;
		case SYS_LSEEK: sys_lseek(vm); break;
		case SYS_ACCESS: sys_access(vm); break;
		case SYS_IOCTL: sys_ioctl(vm);	break;
		case SYS_USELIB: sys_uselib(vm); break;
		case SYS_MMAP: sys_mmap(vm); break;
		case SYS_MUNMAP: sys_munmap(vm); break;
		case SYS_SOCKETCALL: sys_socketcall(vm); break;
		case SYS_FSTAT:	sys_fstat(vm); break;

		default :
			for (i = 1; i <= sys_nargs; i++) {
				switch(i){
					case 1: arg = g_rbx; break;
					case 2: arg = g_rcx; break;
					case 3: arg = g_rdx; break;
					case 4: arg = g_rsi; break;
					case 5: arg = g_rdi; break;
					case 6: arg = g_rbp; break;
				}

				outf("%x", arg);
				if (i < sys_nargs) outf(", ");
			}
	}

	//TODO: Anh - fix this, this is just a temporary way to get return value of syscalls
	// checking vm->nWaitingThreads == 0 is not a good check
	if (vm->nWaitingThreads == 0)
	{
		int tid = get_cur_threadid(vm, 3);
		struct syscall_info info;
		info.syscallno = 0;
		vm_add_waiting_thread(vm, tid, &info);
	}

	// trap right after int0x80 return?
	vm->vmcb->rflags |= X86_RFLAGS_TF;
	vm->vmcb->general1_intercepts |= INTRCPT_IRET;

	outf(" )\n");
}

/*****************************************************************************************/
/************************ HANDLE NESTED PAGE FAULT ********************************/

void __handle_vm_npf (struct vm_info *vm)
{
	// Note for NPF: p410 - 15.24.6 Nested versus Guest Page Faults, Fault Ordering
	u64 errcode = vm->vmcb->exitinfo1;
	print_page_errorcode(errcode);

	//TODO: execute requested access on flash memory (usb drive)
	fatal_failure("Nested page fault!");

//	mmap_4mb(vm->n_cr3, vm->vmcb->exitinfo2, vm->vmcb->exitinfo2, 1);
//	outf("mapping %x on demand\n",vm->vmcb->exitinfo2);

	//Anh - bit 1 of rflags must be set
	vm->vmcb->rflags |= 2;
}

/*****************************************************************************************/
/************************ HANDLE SOFTWARE INTERRUPT & IRET ********************************/

void __handle_vm_swint (struct vm_info *vm)
{
	//HeeDong - handle INTn instruction
	// See AMD manual vol2, page 392
	unsigned long ipaddr_hp;
	ipaddr_hp = glogic_2_hphysic(vm, vm->vmcb->cs.sel, vm->vmcb->rip);

	//Get int vector number from memory
	u8 vector = *((u8 *) (ipaddr_hp + 1));
//	outf("vector for INTn: %x \n", vector);

	switch (vector)
	{
			// SW interrupt
			case 0x80:
				__handle_syscall(vm);
				break;

			// INTn other than 80
			default: ;
	}

	//General handling
	vm->vmcb->eventinj.fields.vector = vector;
	vm->vmcb->eventinj.fields.type = EVENT_TYPE_SWINT;
	vm->vmcb->eventinj.fields.ev = 0;
	vm->vmcb->eventinj.fields.v = 1;

	//Anh - move to next instruction
	vm->vmcb->rip += 2;

//	outf( "\n#SINT vector=%x injected into guest, code: %x\n", vector, vm->vmcb->eventinj.bytes);
}

/******************************************************/

void __handle_vm_iret (struct vm_info *vm)
{
	//After an "iret" event, VMM can get results processed by the linux kernel
	//At present, VMM can get the content of the file read by "sys_read" and the incoming data
	//received by "sys_recvfrom
	//when an "iret" event raised, current process is still in the kernel mode
	//kernel stack esp contains address of the "original" eax which
	//is a system call number. Having this value, VMM can determine
	//which system call raising "iret" and then extract returned data

	if (vm->nWaitingThreads != 0)
	{
		int tid = get_cur_threadid(vm, 0);
		struct syscall_info * info = vm_get_waiting_syscall(vm, tid);
//		outf("iret thread id: %x\n", tid);

		if (info != NULL)	//if found a matching syscall_info
		{
			switch (info->syscallno)
			{
			case 0:
				outf("syscall return: %x\n", vm->vmcb->rax);
				break;
			case SYS_READ: sys_read_iret(vm, info); break; //Handle return of sys_read
			case SYS_OPEN: sys_open_iret(vm, info); break;
			case SYS_SOCKETCALL:
				if (info->socketno == 12) sys_recvfrom_iret(vm, info);
				break;
			default:
				break;
			}

			vm_remove_waiting_thread(vm, tid);
		}
	}

	__skip_intercpt_cur_instr(vm, USER_ITC_IRET);
}

/*****************************************************************************************/
/************************************ HANDLE EXCEPTIONS **********************************/

void __handle_vm_exception (struct vm_info *vm)
{
	//Special handling for GP (syscall)
	switch (vm->vmcb->exitcode)
	{
	case VMEXIT_EXCEPTION_TS:
		print_vmcb_state(vm->vmcb);
		outf("TSS invalid!");
		break;

	case VMEXIT_EXCEPTION_GP:
		print_vmcb_state(vm->vmcb);

		//switch to system call handling code
		vm->vmcb->cs.sel = vm->org_sysenter_cs;
		vm->vmcb->ss.sel = vm->org_sysenter_cs + 8;
		vm->vmcb->rsp = vm->vmcb->sysenter_esp;
		vm->vmcb->rip = vm->vmcb->sysenter_eip;
		return;

	case VMEXIT_EXCEPTION_DB:
//		outf(">> got DB exception\n");

		//check if #DB is caused by single stepping and, if VM enabled single stepping
		// if yes => handle #DB transparently
		if ((vm->vmcb->dr6 & X86_DR6_BS) && (vm->itc_flag & USER_SINGLE_STEPPING)) {
			//TODO: check if program is unpacked?
//			print_vmcb_state(vm->vmcb);
		}

		if (vm->itc_skip_flag) {
//			outf("re-enable skipped interception: %x\n", vm->itc_skip_flag);
			//re-enable skipped interception
			vm_enable_intercept(vm, vm->itc_skip_flag);
			vm->itc_skip_flag = 0;

			// if current process is being tracked and single stepping is requested
			// => should not disable single stepping for the next instruction
			if (!vm->btrackcurrent || !(vm->itc_flag & USER_SINGLE_STEPPING))
				vm_disable_intercept(vm, USER_SINGLE_STEPPING);
		}

		return;

	case VMEXIT_EXCEPTION_PF: //VECTOR_PF
		// check if error was a write protection violation
		// and that UNPACK mode is enabled
		if ((vm->vmcb->exitinfo1 & 1) && (vm->vmcb->exitinfo1 & 2) && (vm->itc_flag & USER_UNPACK))
		{
			outf("USER_UNPACK: guest wrote to %x\n", vm->vmcb->exitinfo2);
			__skip_intercpt_cur_instr(vm, USER_UNPACK); //skip handling next instruction

			return;
		}
		else print_page_errorcode(vm->vmcb->exitinfo1);

		//else => page fault caused by normal guest activity => inject exception
		vm->vmcb->cr2 = vm->vmcb->exitinfo2;
		break;
	}

	//Anh - See AMD manual vol2, page 392 & 385
	int vector =  vm->vmcb->exitcode - VMEXIT_EXCEPTION_DE;

	//General handling
	vm->vmcb->eventinj.fields.vector = vector;
	vm->vmcb->eventinj.fields.type = EVENT_TYPE_EXCEPTION;
	vm->vmcb->eventinj.fields.ev = 1;
	vm->vmcb->eventinj.fields.v = 1;
	vm->vmcb->eventinj.fields.errorcode = vm->vmcb->exitinfo1;

	outf( "\n#Exception vector=%x injected into guest, code: %x\n", vector, vm->vmcb->eventinj.bytes);

//	outf( "\nValue of EXITINTINFO: %x\n", vm->vmcb->exitintinfo);
}

/*****************************************************************************************/
/********************************* HANDLE TASKSWITCH **************************************/

void __handle_cr3_write (struct vm_info *vm) {
//	outf(">> write cr3\n");

	//Anh - popf is the final instruction in Linux's switch_to()
	// after new stack has been set-up
	vm->vmcb->general1_intercepts |= INTRCPT_POPF;

//	u32 * ip_hp = (u32 *) glogic_2_hphysic(vm, vm->vmcb->cs.sel, vm->vmcb->rip);
//	outf("opcode:%x\n", *ip_hp);
//	outf("rax:%x\n", vm->vmcb->rax);

	//skip intercepting current mov cr3 instruction, avoid looping back to VMM
	__skip_intercpt_cur_instr(vm, USER_ITC_TASKSWITCH);
}

/***************************************************/

void __handle_popf (struct vm_info *vm) {
	char * pname = get_cur_process_name(vm,0);
	vm->vmcb->general1_intercepts &= ~INTRCPT_POPF;

	// if the next process is in tracked list => enable interceptions
	// otherwise disable them
	if (vm_is_process_tracked(vm, pname)) {
//		outf("process tracked\n");
		int flags = vm->itc_flag & ~((int) USER_ITC_TASKSWITCH);
		vm_enable_intercept(vm, flags);

		vm->btrackcurrent = 1;
	}
	else {
		int flags = vm->itc_flag & ~((int) USER_ITC_TASKSWITCH);
		// disable everything but TASKSWITCH interception
		vm_disable_intercept(vm, flags);

		vm->btrackcurrent = 0;
	}
}

/***************************************************/
//void __handle_task_switch (struct vm_info *vm) {
//	outf("Task switch\n");
//	get_cur_process_name(vm);
//}

/*****************************************************************************************/
// Anh - Handle user request to enable some features
void user_enable_intercept(struct vm_info *vm, int flags)
{
	// taskswitch interception needs special care
	// it acts as the key to enable all other interceptions for specific proceses
	if (flags & USER_ITC_TASKSWITCH) {
		outf("User enable taskswitch interception\n");

		vm->vmcb->cr_intercepts |= INTRCPT_WRITE_CR3;

		//g_rdx points to name of the process that needed to be tracked
		char * pname = (char *) glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rdx);
		//outf("add process to tracking list: %s\n", pname);
		vm_add_tracked_process(vm, pname);
	}

	//set interception flag in vm_info to corresponding value
	vm->itc_flag |= flags;
}

/**********************************************************/
void user_disable_intercept(struct vm_info *vm, int flags)
{
	if (flags & USER_ITC_TASKSWITCH) {
		outf("User disable taskswitch interception\n");

		char * pname = (char *) glogic_2_hphysic(vm, vm->vmcb->ds.sel, g_rdx);
		outf("remove process from tracking list: %s\n", pname);
		vm_remove_tracked_process(vm, pname);

		if (vm->nTrackedProcess == 0) vm->vmcb->cr_intercepts &= ~INTRCPT_WRITE_CR3;
	}

	//set interception flag in vm_info to corresponding value
	vm->itc_flag &= ~flags;
}

/**********************************************************/
/*
extern void read_sector_asm();

void user_test(int flags, struct vm_info *vm) {
	switch (flags) {
	case USER_TEST_SWITCHMODE:
		__asm__ volatile (
				"mov %0, %%eax;"
				"mov %1, %%ecx;"
				"call read_sector_asm"
				:
				:"g" (1), "g" (1000)
				:"%eax", "%ecx"
		);

		break;
	}
}
*/
/**********************************************************/

//Handle commands from usermode, sent via vmmcall instruction
void __handle_vm_vmmcall (struct vm_info *vm)
{
	vm->vmcb->rip+=3; // skip the vmmcall instruction
	outf("GOT A VMMCALL!!! eax = %x, ", vm->vmcb->rax);
	outf("ebx = %x, ", g_rbx);
	outf("ecx = %x, ", g_rcx);
	outf("edx = %x\n", g_rdx);

	switch (g_rbx) {
	case USER_CMD_ENABLE:
		user_enable_intercept(vm, g_rcx);
		break;

	case USER_CMD_DISABLE:
		user_disable_intercept(vm, g_rcx);
		break;

	case USER_CMD_TEST:
		//user_test(g_rcx, vm);
		break;
	}
}

/*****************************************************************************************/
/******************************* HANDLE MSR ACCESS ****************************************/

//HeeDong - handle WRMSR intercept, mainly for SYSENTER intercept
void __handle_vm_wrmsr (struct vm_info *vm)
{
//	outf( "\n\nvalue of ECX: %x\n\n", g_rcx );

	//HeeDong - write what should have been written without intercept
	//and increase the rip by 2 because wrmsr instruction is 2 bytes
	switch (g_rcx)	//g_rcx contains MSR number
	{
	case MSR_IA32_SYSENTER_CS:	//sysenter_cs_msr
		vm->org_sysenter_cs = ((u64) g_rdx << 32) + (0xFFFFFFFF & vm->vmcb->rax);
		vm->vmcb->sysenter_cs = SYSENTER_CS_FAULT;
		vm->vmcb->rip += 2;
		break;

//	case MSR_SYSENTER_ESP: //sysenter_esp_msr
//		vm->org_sysenter_esp = ((u64) g_rdx << 32) + (0xFFFFFFFF & vm->vmcb->rax);
//		vm->vmcb->sysenter_esp = vm->org_sysenter_esp;
////		vm->vmcb->sysenter_esp = SYSENTER_ESP_FAULT;
//		vm->vmcb->rip += 2;
//		break;
//
//	case MSR_SYSENTER_EIP: //sysenter_eip_msr
//		vm->org_sysenter_eip = ((u64) g_rdx << 32) + (0xFFFFFFFF & vm->vmcb->rax);
//		vm->vmcb->sysenter_eip = vm->org_sysenter_eip;
////		vm->vmcb->sysenter_eip = SYSENTER_EIP_FAULT;
//		vm->vmcb->rip += 2;
//		break;

	default:;
		outf( "\n\nShould NOT REACH HERE!!!\n\n" );
	}
}


/*****************************************************************************************/
/********************************** MAIN FUNCTION ****************************************/
/*****************************************************************************************/

void handle_vmexit (struct vm_info *vm)
{
//	outf("**** ");
	outf("**** #VMEXIT - exit code: %x\n", (u32) vm->vmcb->exitcode);
	print_vmcb_state(vm->vmcb);
//	print_vmexit_exitcode (vm->vmcb);

//	if (vm->vmcb->exitintinfo.fields.type == EVENT_TYPE_EXCEPTION)
//		fatal_failure("Pending guest exception!\n");

	switch (vm->vmcb->exitcode)
	{
		case VMEXIT_MSR:
			if (vm->vmcb->exitinfo1 == 1) __handle_vm_wrmsr (vm);
			break;
		case VMEXIT_EXCEPTION_DE ... VMEXIT_EXCEPTION_XF:
			__handle_vm_exception(vm); break;

		//software interrupt (int n)
		case VMEXIT_SWINT: __handle_vm_swint(vm); break;
		//nested page fault
		case VMEXIT_NPF: __handle_vm_npf (vm); break;
		//vmmcall
		case VMEXIT_VMMCALL: __handle_vm_vmmcall (vm); break;
		//iret
		case VMEXIT_IRET: __handle_vm_iret(vm); break;
		//write to cr3
		case VMEXIT_CR3_WRITE: __handle_cr3_write(vm);break;
		case VMEXIT_POPF: __handle_popf(vm); break;
//		case VMEXIT_TASK_SWITCH: __handle_task_switch(vm); break;
	}
}
