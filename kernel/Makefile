MAVMM = mavmm
SOS  = sos

TOP_DIR     = ..
TOOLS_DIR   = ${TOP_DIR}/tools
INCLUDE_DIR = ${TOP_DIR}/include

CC      = gcc
NM      = nm
LD      = ld
MKELF32 = ${TOOLS_DIR}/mkelf32

INCLUDES = -I${INCLUDE_DIR}
CFLAGS   = -Wall ${INCLUDES} -fPIC -nostdinc -nostdlib -fno-builtin -iwithprefix include 

# http://linux.die.net/man/1/gcc
# -Wall: all warnings
# -fPIC
#       If supported for the target machine, emit position-independent code, suitable for dynamic linking and avoiding any limit on the size of the global offset
#       table.  This option makes a difference on the m68k, PowerPC and SPARC.
# -nostdinc
#    	Do not search the standard system directories for header files. Only the directories you have specified with -I options (and the directory of the current file, if #	appropriate) are searched. 
# -nostdlib
#    	Do not use the standard system startup files or libraries when linking. 
# -iwithprefix dir
#    	Append dir to the prefix specified previously with -iprefix, and add the resulting directory to the include search path. 
# -fno-stack-protector: disable stack protector (which automatically inserts protection code into an application at compilation time)

HDRS  = ${INCLUDE_DIR}/types.h ${INCLUDE_DIR}/multiboot.h ${INCLUDE_DIR}/string.h ${INCLUDE_DIR}/printf.h ${INCLUDE_DIR}/failure.h \
	${INCLUDE_DIR}/bitops.h ${INCLUDE_DIR}/msr.h ${INCLUDE_DIR}/e820.h ${INCLUDE_DIR}/cpufeature.h ${INCLUDE_DIR}/cpu.h \
	${INCLUDE_DIR}/page.h ${INCLUDE_DIR}/svm.h \
	${INCLUDE_DIR}/vmcb.h ${INCLUDE_DIR}/vm.h  ${INCLUDE_DIR}/pmem_layout.h \
	${INCLUDE_DIR}/vmm.h ${INCLUDE_DIR}/alloc.h ${INCLUDE_DIR}/segment.h

COMMON_OBJECTS = string.o printf.o failure.o e820.o

# [???] boot.o must be the head of list
MAVMM_OBJECTS   = boot.o ${COMMON_OBJECTS} cpu.o segment.o \
		  alloc.o svm.o svm_asm.o page.o vmcb.o vm.o setup.o serial.o intercept.o \
		  vmx.o vmx_asm.o vmcs.o

SOS_OBJECTS    = sos_boot.o ${COMMON_OBJECTS} sos.o

#load at 128MB, used for both -T and -Ttext options
#LOAD_BASE_ADDR = 0x8000000

TEXT_OFFSET    = 0xFFFF830000200000
LOAD_BASE_ADDR = 0x200000
MAVMM_LD_BASE  = 0x8000000

all: ${MAVMM}

${MAVMM}: ${MAVMM_OBJECTS} ${MAVMM}.lds
	${LD} ${LDFLAGS} -T ${MAVMM}.lds -o ${MAVMM}-syms -Ttext ${TEXT_OFFSET} ${MAVMM_OBJECTS}
#create an elf version of the VMM
#	mkelf32 <in-image> <out-image> <load-base> <final-exec-addr>
# 	cut -c1-8 take the first 8 characters 
	${MKELF32} $@-syms $@ ${LOAD_BASE_ADDR} \
	  `${NM} --numeric-sort ${MAVMM}-syms | tail -n 1 | sed -e 's/^\([^ ]*\).*/0x\1/'`	#third param of mkelf32

%.o: %.c ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

%.o: %.S ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${COMMON_OBJECTS} ${MAVMM_OBJECTS} ${MAVMM} ${MAVMM}-syms
