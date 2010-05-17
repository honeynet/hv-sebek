#ifndef __SVM_H__
#define __SVM_H__

#include "types.h"
#include "cpu.h"
#include "vmcb.h"

#ifndef __ASSEMBLY__

//HeeDong - variables to store and load registers of Guest
// we don't need to store eax because eax is stored inside vmcb.rax
// and it will be loaded / stored with VMLOAD / VMSAVE (remember to use these instructions)
u32 g_ebp;
u32 g_ebx;
u32 g_ecx;
u32 g_edx;
u32 g_esi;
u32 g_edi;

extern void __init enable_svm ( struct cpuinfo_x86 *c );
extern void svm_launch ();

#endif

#endif /* __SVM_H__ */
