#!/bin/bash
# Author: Nabil Schear
#

#change these paths to be your system commands
LOSETUP=/sbin/losetup
MOUNT=/bin/mount
UMOUNT=/bin/umount

# this is the offset into the partition where the partition table
# begins.  This is set when you create the partition with fdisk.
# it is the number of sectors times the block size 512
OFFSET=32256

# this is the loop device that this script will use. if loop0 is used by
# something else then set this to loop1-9
LOOPDEV=/dev/loop0

if [ $# -lt 1 ]
then
  echo "Usage: `basename $0` {mount|umount} [imagename]" >&2
  exit $NOARGS
fi

if [ "$1" == mount ]
then
    if [ $# -lt 3 ]
    then
	echo "Usage: `basename $0` {up|down} [imagename] [mount point]" >&2
	echo "       please specify an image file to mount and a mount point">&2
	exit $NOARGS
    fi

    echo "Mounting image $2 on loopback"
    $LOSETUP -o $OFFSET $LOOPDEV $2
    echo "Mounting file system on $3"
    $MOUNT -o loop $LOOPDEV $3
else
    if [ $# -lt 2 ]
    then
	echo "Usage: `basename $0` {up|down} [imagename] [mount point]" >&2
	echo "       please specify a mount point to unmount">&2
	exit $NOARGS
    fi
    echo "Unmounting image from loop0"
    $UMOUNT $2
    $LOSETUP -d $LOOPDEV
fi

