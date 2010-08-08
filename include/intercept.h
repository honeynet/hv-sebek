#ifndef _H_INTERCEPT_H_
#define _H_INTERCEPT_H_

/* To parse the exit code in case of an Intercept
and perform actions based it */

#include "vm.h"

extern void handle_vmexit (struct vm_info *vm);

/*
extern void __handle_syscall ( struct vm_info *vm );
extern void __handle_vm_npf (struct vm_info *vm);
extern void __handle_vm_swint (struct vm_info *vm);
extern void __handle_vm_iret (struct vm_info *vm);
extern void __handle_vm_exception (struct vm_info *vm);
extern void __handle_cr3_write (struct vm_info *vm);
extern void __handle_popf (struct vm_info *vm);
extern void __handle_vm_vmmcall (struct vm_info *vm);
extern void __handle_vm_wrmsr (struct vm_info *vm);
*/

/* [REF] AMD64 manual Vol. 2, Appendix B */

//Flags for CRn access interception
#define _INTRCPT_WRITE_CR0_OFFSET	16
#define INTRCPT_WRITE_CR3 			(1 << (_INTRCPT_WRITE_CR0_OFFSET + 3))

//Flags for exception intercept word
// 1 << vector associated with the exception - VMCB + 8h
#define INTRCPT_DB			(1 << 1)	//Debug exception
#define INTRCPT_TS			(1 << 10)	//Invalid TSS
#define INTRCPT_NP			(1 << 11)	//Segment-Not-Present Exception
#define INTRCPT_GP			(1 << 13)	//General protection
#define INTRCPT_PF			(1 << 14) 	//Pagefault exception

//Flags for FIRST general intercept word - VMCB + 0Ch
#define INTRCPT_INTR		(1 << 0)
#define INTRCPT_READTR		(1 << 9)
#define INTRCPT_IRET		(1 << 20)
#define INTRCPT_POPF		(1 << 17)
#define INTRCPT_INTN		(1 << 21)
#define INTRCPT_IO     		(1 << 27)
#define INTRCPT_MSR   		(1 << 28)
#define INTRCPT_TASKSWITCH	(1 << 29)

//Flags for SECOND general intercept word - VMCB + 010h
#define INTRCPT_VMRUN     (1 << 0)
#define INTRCPT_VMMCALL   (1 << 1)

#define TRACE_FILE		001	/* Trace file-related syscalls. */
#define TRACE_IPC		002	/* Trace IPC-related syscalls. */
#define TRACE_NETWORK	004	/* Trace network-related syscalls. */
#define TRACE_PROCESS	010	/* Trace process-related syscalls. */
#define TRACE_SIGNAL	020	/* Trace signal-related syscalls. */
#define TRACE_DESC		040	/* Trace file descriptor-related syscalls. */

/* Define these shorthand notations to simplify the syscallent files. */
#define TD TRACE_DESC
#define TF TRACE_FILE
#define TI TRACE_IPC
#define TN TRACE_NETWORK
#define TP TRACE_PROCESS
#define TS TRACE_SIGNAL

// values which would create fault when the guest make a syscall
#define SYSENTER_CS_FAULT		0
//#define SYSENTER_ESP_FAULT	0
//#define SYSENTER_EIP_FAULT	0

#define SYS_READ		3
#define SYS_WRITE		4
#define SYS_OPEN		5
#define SYS_CLOSE		6
#define SYS_WAITPID		7
#define SYS_LINK		9
#define SYS_UNLINK		10
#define SYS_EXECVE		11
#define SYS_LCHOWN		16
#define SYS_LSEEK		19
#define SYS_ACCESS		33
#define SYS_IOCTL		54
#define SYS_USELIB		86
#define SYS_MMAP		90
#define SYS_MUNMAP		91
#define SYS_SOCKETCALL	102
#define SYS_FSTAT		108

/*Define various values of the "call" parameter in sys_socketcall*/
#define SYS_SOCKET    		1        /* sys_socket(2)        */
#define SYS_BIND    		2        /* sys_bind(2)            */
#define SYS_CONNECT    		3        /* sys_connect(2)        */
#define SYS_LISTEN    		4        /* sys_listen(2)        */
#define SYS_ACCEPT    		5        /* sys_accept(2)        */
#define SYS_GETSOCKNAME   	6        /* sys_getsockname(2)        */
#define SYS_GETPEERNAME    	7        /* sys_getpeername(2)        */
#define SYS_SOCKETPAIR    	8        /* sys_socketpair(2)        */
#define SYS_SEND    		9        /* sys_send(2)            */
#define SYS_RECV    		10        /* sys_recv(2)            */
#define SYS_SENDTO    		11        /* sys_sendto(2)        */
#define SYS_RECVFROM    	12        /* sys_recvfrom(2)        */
#define SYS_SHUTDOWN    	13        /* sys_shutdown(2)        */
#define SYS_SETSOCKOPT    	14        /* sys_setsockopt(2)        */
#define SYS_GETSOCKOPT    	15        /* sys_getsockopt(2)        */
#define SYS_SENDMSG    		16        /* sys_sendmsg(2)        */
#define SYS_RECVMSG    		17        /* sys_recvmsg(2)        */

//define struct sockaddr for processing network system calls
struct sockaddr {
	unsigned short sa_family;		//address family, AF_
	char sa_data[14];				//14 bytes of protocol address
};

struct in_addr {
	unsigned long s_addr;			//load with inet_pton()
};

//IPv4 AF_INET socket
struct sockaddr_in {
	short ins_family;				//AF_INET
	unsigned short sin_port;		//ntons(212)
	struct in_addr sin_addr;
	char sin_zero[8];				//zero this if you want
};

// define file access mode (unistd.h)
#define R_OK    4               // Test for read permission
#define W_OK    2               // Test for write permission
#define X_OK    1               // Test for execute permission
#define F_OK    0               // Test for existence


#endif
