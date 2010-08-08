#ifndef __SVM_H__
#define __SVM_H__

#include "types.h"
#include "cpu.h"
#include "vmcb.h"
#include "vm.h"

#ifndef __ASSEMBLY__

extern void init_amd ( struct vm_info *vm, struct cpuinfo_x86 *cpuinfo );
extern void svm_launch ();

#endif

#endif /* __SVM_H__ */
