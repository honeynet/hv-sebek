TARGET = mavmm-u

all: ${TARGET}.c
	gcc-3.4 -m32 -o ${TARGET} ${TARGET}.c
	cp -v ${TARGET} ../test/
	
clean:
	rm -f *.o *.~ ${TARGET}