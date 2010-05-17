#cd invaders
#./compile.sh
#cd ..

#rm -f kernel/mavmm
#make clean
#make

# check number of arguments
if [ $# -gt 0 ]; then
  if [ "$1" = "usb" ];  then
    echo "Make USB boot disk"
    sudo ./mkusb.sh /dev/sdb kernel/mavmm
    exit 0
  fi
else
  sudo ./mkdisk.sh mavmm.hdd kernel/mavmm	
fi

if [ $? -ne 0 ]; then
    echo "Error building disk image!"
    exit -1
fi

. ~/Desktop/Simnow.sh

#sudo ./mkdisk.sh mavmm.hdd kernel/mavmm kernel/elf-sos
#sudo ./updatedisk.sh mavmm.hdd invaders/mbi-invaders boot/mbi-invaders

#cd tools/mkelfImage
#./configure
#make
#cd ..
#cd .. 

#rm -f dsl/boot/isolinux/elf-nabix
#./tools/mkelfImage/objdir/sbin/mkelfImage --kernel dsl/boot/isolinux/linux24 --mavmm dsl/boot/isolinux/elf-nabix
#sudo ./updatedisk.sh mavmm.hdd dsl/boot/isolinux/elf-nabix boot/isolinux
