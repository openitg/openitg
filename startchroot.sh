#!/bin/sh

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup and enter the chroot environment"
        exit 1;
fi

if [ ! -d "$1" ]; then
    echo "Usage: startchroot.sh path-to-chroot"
    exit 1;
fi

do_chroot_mounts() {
    chroot_base=$1
    mount -o bind /proc $chroot_base/proc || return 1
    mount -o bind /sys $chroot_base/sys || return 1
    mount -o bind $(pwd) $chroot_base/root/openitg-dev || return 1
}

chroot_unmount() {
    chroot_base=$1
    umount $chroot_base/proc
    umount $chroot_base/sys
    umount $chroot_base/root/openitg-dev
    return 0
}

error_out() {
    chroot_base=$1
    cat <<!
Error mounting one of the mounts.  You should manually run:

mount -o bind /proc $chroot_base/proc
mount -o bind /sys $chroot_base/sys
mount -o bind $(pwd) $chroot_base/root/openitg-dev

And check if any fail, then run:

sudo chroot $chroot_base

Be sure to unmount these folders after leaving the chroot.
!
    exit 1;
}

do_chroot_mounts $1 || error_out $1
PATH="/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" chroot $1
chroot_unmount $1
