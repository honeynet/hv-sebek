#!/bin/bash
#

# set this to 1 if you want to include ttylinux in the boot image.
TTYLINUX=1
TINYCORE=1
PUPPY=0
INSERT=0
DSL=0


# check the arguments
if [ $# -lt 2 ]
then
  echo "Usage: `basename $0` usb_device mavmm-binary" >&2
  exit $NOARGS
fi

echo "==========================================================="

# change these paths to be your system commands (if they are different)
FDISK=/sbin/fdisk
MOUNT=/bin/mount
UMOUNT=/bin/umount
GRUB=/usr/sbin/grub
MKFS=/sbin/mkfs.vfat

# USB device
USB_DEVICE=$1
USB_PARTITION_NUMBER=1
USB_PARTITION=$USB_DEVICE$USB_PARTITION_NUMBER
USB_HD_GRUB_STYLE=hd1
I_AM_READY=1    # change this value to 1, just make sure you read me.

if [ $I_AM_READY -ne 1 ]
then
  echo "Edit I_AM_READY in USB device section of mkusb.sh before continue!!!" >&2
  exit $NOARGS
fi

# this path needs to point to where the grub stage1-2 boot sectors
# are stored.  it is given below for my system.  You can also just set
# this to /boot/grub and use the system stage files
GRUB_LIB=/boot/grub

echo "using $GRUB_LIB for grub location"

# point to the menu.lst file.  this will be copied to grub.conf or
# menu.lst eventually.
MENU_LST=menu.lst
MENU_LST_AD=menu_ad.lst	#Addition grub menu list for current OSes

# function to clean up after a run
function cleanup {
    echo "Unmounting USB after error"
     $UMOUNT /mnt
    if [ $? -ne 0 ];    then
        echo "error"
    fi
   }

# unmount usb for sure
$MOUNT $USB_PARTITION
if [ $? -ne 0 ];  then
        echo "..."
fi


# HERE THE SCRIPT ACTUALLY STARTS!
echo "creating partition table with fdisk..."
$FDISK $USB_DEVICE <<EOF
o
p
n
p
1


p
t
c
a
1
p
w
EOF

echo "Try umount before make file system, because Ubuntu hase a deamon that automaticly mount USB"
$UMOUNT $USB_PARTITION

echo "Creating vfat (FAT32) file system"
$MKFS $USB_PARTITION
if [ $? -ne 0 ]
then
    echo "mkfs error"
    exit -1
fi

echo "Mounting file system on /mnt"
$MOUNT $USB_PARTITION /mnt
if [ $? -ne 0 ]
then
    echo "error"
    exit -1
fi

echo "Setting up GRUB"
mkdir /mnt/boot
mkdir /mnt/boot/grub
if [ $? -ne 0 ]
then
    echo "error"
    cleanup
    exit -1
fi

echo "copying grub stage and device mapped files"
cp -v  $GRUB_LIB/* /mnt/boot/grub/
rm -f  /mnt/boot/grub/menu.lst

if [ $? -ne 0 ]
then
    echo "error"
    cleanup
    exit -1
fi

echo "copying grub configuration"
cp ./$MENU_LST /mnt/boot/grub/menu.lst
cat ./$MENU_LST_AD >> /mnt/boot/grub/menu.lst

if [ $? -ne 0 ]
then
    cleanup
    echo "error"
    exit -1
fi

cp ./$MENU_LST /mnt/boot/grub/grub.conf
cat ./$MENU_LST_AD >> /mnt/boot/grub/grub.conf
if [ $? -ne 0 ]
then
    cleanup
    echo "error"
    exit -1
fi

echo "copying MAVMM $2 to image/boot"
cp -v $2 /mnt/boot/
if [ $? -ne 0 ]
then
    cleanup
    echo "error"
    exit -1
fi

#Copy test programs, such as syscall, io test...
echo "copying test programs..."
mkdir /mnt/boot/
cp -vR test/ /mnt/

#Copy TTYLinux
if [ $TTYLINUX -eq 1 ]
then
    echo "copying TTYLinux..."
    mkdir /mnt/boot/ttylinux
    cp -v ttylinux/filesys.gz /mnt/boot/ttylinux/
    cp -v ttylinux/vmlinuz /mnt/boot/ttylinux/
fi

echo "copying testOS to image/boot"
cp -v testos/testos /mnt/boot/
if [ $? -ne 0 ]
then
    cleanup
    echo "error copying testOS"
    exit -1
fi

if [ $DSL -eq 1 ]
then
    echo "copying DSL Linux..."
    cp -vR dsl/* /mnt/
fi

if [ $PUPPY -eq 1 ]
then
    echo "copying Puppy Linux..."
    cp -vR puppy/* /mnt/
fi

if [ $INSERT -eq 1 ]
then
    echo "copying INSERT Linux..."
    cp -vR INSERT/* /mnt/
fi

if [ $TINYCORE -eq 1 ]
then
    echo "copying TinyCore Linux..."
    mkdir /mnt/boot/tinycore
    cp -vR tinycore/boot/bzImage /mnt/boot/tinycore/
    cp -vR tinycore/boot/tinycore.gz /mnt/boot/tinycore/
fi

echo "Unmounting usb"
$UMOUNT /mnt
if [ $? -ne 0 ]
then
    echo "error"
    exit -1
fi

echo "Installing GRUB to MBR of USB"
$GRUB <<EOF
root ($USB_HD_GRUB_STYLE,0)
setup ($USB_HD_GRUB_STYLE)
quit
EOF

if [ $? -ne 0 ]
then
    echo "error"
    exit -1
fi

echo "SUCCESS creating MAVMM USB disk"
