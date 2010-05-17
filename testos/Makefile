TESTOS = testos

TOP_DIR     = ..
TOOLS_DIR   = ${TOP_DIR}/tools
INCLUDE_DIR = ${TOP_DIR}/include

CC      = gcc
NM      = nm
LD      = ld
MKELF32 = ${TOOLS_DIR}/mkelf32

INCLUDES = -I${INCLUDE_DIR}
CFLAGS   = -m32 -Wall ${INCLUDES} -fPIC -nostdinc -nostdlib -fno-builtin -iwithprefix include -fno-stack-protector
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

TESTOS_OBJECTS  = testos.o testosc.o
LOAD_BASE_ADDR 	= 0
	         
all: ${TESTOS}

${TESTOS}: ${TESTOS_OBJECTS} ${TESTOS}.lds
	${LD} ${LDFLAGS} -T ${TESTOS}.lds -o ${TESTOS}-syms -Ttext 0x100000 ${TESTOS_OBJECTS} 
	${MKELF32} $@-syms $@ 0x100000 \
	  `${NM} --numeric-sort ${TESTOS}-syms | tail -n 1 | sed -e 's/^\([^ ]*\).*/0x\1/'`

%.o: %.c ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

%.o: %.S ${HDRS} Makefile
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${COMMON_OBJECTS} ${TESTOS_OBJECTS} ${TESTOS} ${TESTOS}-syms