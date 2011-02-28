#!/bin/sh

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup the chroot environment"
        exit 1;
fi

if [ ! -d "$1" ]; then
    echo "Usage: startchroot.sh path-to-chroot"
    exit 1;
fi

mount -o bind /proc $1/proc && mount -o bind /sys $1/sys && mount -o bind `pwd` $1/root/openitg-dev && echo "To enter the chroot, run 'sudo chroot $1'" && exit 0;

cat <<! 
    Error mounting one of the mounts.  You should manually run:

    mount -o bind /proc $1/proc
    mount -o bind /sys $1/sys
    mount -o bind `pwd` $1/root/openitg-dev
   
    And check if any fail, then run:

    sudo chroot $1
!
exit 1;

