TARGET = mavmm

all: ${TARGET}

${TARGET}:
	make -C kernel
	make -C testos
	make -C user
	make -C hostfs	#fat32 vmm host file system
		
clean:
	make -C kernel clean
	make -C testos clean
	make -C user clean
	make -C hostfs clean	
	rm *.~

inv:
	make -C invaders
