rm -f kernel/mavmm
#make clean
make
sudo ./updatedisk.sh mavmm.hdd kernel/mavmm boot

if [ $? -ne 0 ] 
then
    echo "Error updating disk image!"
    exit -1
fi

. ~/Desktop/Simnow.sh