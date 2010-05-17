#!/bin/bash
#

# check the arguments
if [ $# -lt 3 ]
then
  echo "Usage: `basename $0` image-name file dest-dir" >&2
  exit $NOARGS
fi

echo "==========================================================="

# change these paths to be your system commands (if they are different)
LOSETUP=/sbin/losetup
MOUNT=/bin/mount
UMOUNT=/bin/umount

# here's the info for the disk image size and configuration
# to make it bigger probably just change the number of cylinders
CYLINDERS=125
HEADS=16
SECTORS=63

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

echo "Associating image to loopback device"
$LOSETUP -v -o `expr $SECTORS \* 512` /dev/loop0 $1
if [ $? -ne 0 ]
then
    echo "error"
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

echo "copying $2 to disk image directory $3"
cp -v $2 /mnt/$3
if [ $? -ne 0 ] 
then
    cleanup
    echo "error"
    exit -1
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

echo "SUCCESS updating MAVMM disk image in file: $1"
