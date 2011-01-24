#!/bin/sh

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup the chroot environment"
        exit 1;
fi

if [[ ! -d "$1" ]]; then
    echo "Usage: unchroot.sh path-to-chroot"
    exit 1;
fi

umount $1/proc && umount $1/sys && umount $1/root/openitg-dev && echo "It is now safe to rm -rf $1" && exit 0;

cat <<! 
    Error unmounting one of the mounts.  You should manually run:

    umount $1/proc
    umount $1/sys
    umount $1/root/openitg-dev
    
    Only after those are safely unmounted can you remove your chroot.
!
exit 1;

