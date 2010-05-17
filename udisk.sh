#cd invaders
#./compile.sh
#cd ..

rm -f kernel/mavmm
#make clean
make
sudo ./mkdisk.sh mavmm.hdd kernel/mavmm

if [ $? -ne 0 ] 
then
    echo "Error building disk image!"
    exit -1
fi
