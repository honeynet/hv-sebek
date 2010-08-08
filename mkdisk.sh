#!/bin/bash
#

# set this to 1 if you want to include ttylinux in the boot image.
TTYLINUX=1
TINYCORE=0
PUPPY=0
INSERT=0
DSL=0


# check the arguments
if [ $# -lt 2 ]
then
  echo "Usage: `basename $0` image-name mavmm-binary" >&2
  exit $NOARGS
fi

echo "==========================================================="

# change these paths to be your system commands (if they are different)
DD=/bin/dd
FDISK=/sbin/fdisk
LOSETUP=/sbin/losetup
MOUNT=/bin/mount
UMOUNT=/bin/umount
GRUB=/usr/sbin/grub
MKFS=/sbin/mkfs.ext2

# this path needs to point to where the grub stage1-2 boot sectors
# are stored.  it is given below for my system.  You can also just set
# this to /boot/grub and use the system stage files
if uname -r | grep -q el5
then
	# use this version if you're on Anh's machine
	GRUB_LIB=/usr/share/grub/x86_64-redhat/
else
	GRUB_LIB=/usr/lib/grub/x86_64-pc/
fi

echo "using $GRUB_LIB for grub location"

# here's the info for the disk image size and configuration
# to make it bigger probably just change the number of cylinders
CYLINDERS=125
HEADS=16
SECTORS=63

# point to the menu.lst file.  this will be copied to grub.conf or 
# menu.lst eventually
MENU_LST=menu.lst

# function to clean up after a run
function cleanup {
    echo "Cleaning up loopback after error"
    echo "     Unmounting image"
    $UMOUNT /mnt
    if [ $? -ne 0 ] 
    then
	$LOSETUP -d /dev/loop0
	echo "error"
	exit -1
    fi
    
    echo "     Removing loopback device"
    $LOSETUP -d /dev/loop0
    if [ $? -ne 0 ] 
    then
	echo "error"
	exit -1
    fi
}

# HERE THE SCRIPT ACTUALLY STARTS!

echo "creating empty file $1 for disk image"
$DD if=/dev/zero of=$1 count=`expr $CYLINDERS \* $HEADS \* $SECTORS`
if [ $? -ne 0 ] 
then
    echo "error"
    exit -1
fi

echo "creating partition table with fdisk..."
$FDISK $1 <<EOF
x
c
$CYLINDERS
h
$HEADS
s
$SECTORS
r
n
p
1
1
$CYLINDERS
a
1
w
EOF
$FDISK -l -u $1

echo "Associating image to loopback device"
$LOSETUP -v -o `expr $SECTORS \* 512` /dev/loop0 $1
if [ $? -ne 0 ]
then
    echo "error"
    exit -1
fi

echo "Creating file system"
$MKFS /dev/loop0
if [ $? -ne 0 ] 
then
    echo "mkfs error"
    $LOSETUP -d /dev/loop0
    exit -1
fi

echo "Mounting file system on /mnt"
$MOUNT -o loop /dev/loop0 /mnt
if [ $? -ne 0 ] 
then
    echo "error"
    $LOSETUP -d /dev/loop0
    exit -1
fi

echo "Setting up GRUB"
mkdir -p /mnt/boot/grub
if [ $? -ne 0 ] 
then
    echo "error"
    cleanup
    exit -1
fi

echo "copying grub stage files"
cp $GRUB_LIB/stage1 $GRUB_LIB/stage2 $GRUB_LIB/e2fs_stage1_5 /mnt/boot/grub
if [ $? -ne 0 ] 
then
    echo "error"
    cleanup
    exit -1
fi

echo "copying grub configuration"
cp ./$MENU_LST /mnt/boot/grub/menu.lst
if [ $? -ne 0 ] 
then
    cleanup
    echo "error"
    exit -1
fi

cp ./$MENU_LST /mnt/boot/grub/grub.conf
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

echo "Unmounting image"
$UMOUNT /mnt
if [ $? -ne 0 ] 
then
    $LOSETUP -d /dev/loop0
    echo "error"
    exit -1
fi
    
echo "removing loopback device"
$LOSETUP -d /dev/loop0
if [ $? -ne 0 ] 
then
    echo "error"
    exit -1
fi

echo "Installing GRUB"
$GRUB --device-map=/dev/null --config-file=menu.lst <<EOF
device (hd0) $1
geometry (hd0) $CYLINDERS $HEADS $SECTORS
root (hd0,0)
setup (hd0)
quit
EOF

if [ $? -ne 0 ] 
then
    echo "error"
    exit -1
fi

echo "SUCCESS creating MAVMM disk image in file: $1"
