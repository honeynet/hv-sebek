# VMM FAT32 host fily system

HOSTFS = hostfs

TOP_DIR     = ..
TOOLS_DIR   = ${TOP_DIR}/tools
INCLUDE_DIR = ${TOP_DIR}/include

CC      = gcc
NM      = nm
LD      = ld
MKELF32 = ${TOOLS_DIR}/mkelf32
INCLUDES = -I${INCLUDE_DIR}

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

CFLAGS   = -m32 -Wall ${INCLUDES} -fPIC -nostdinc -nostdlib -fno-builtin -iwithprefix include -fno-stack-protector
LDFLAGS	= -m elf_i386

COMMON_OBJECTS = ../kernel/string.o ../kernel/printf.o ../kernel/failure.o ../kernel/e820.o ../kernel/serial.o
HOSTFS_OBJECTS  = fat32.o fat32_ui.o mode.o
	         
all: ${HOSTFS}

${HOSTFS}: ${HOSTFS_OBJECTS} ${COMMON_OBJECTS}
	${LD} ${LDFLAGS} -o ${HOSTFS} ${HOSTFS_OBJECTS} ${COMMON_OBJECTS}

%.o: %.c ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

%.o: %.S ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${HOSTFS} ${HOSTFS_OBJECTS}

