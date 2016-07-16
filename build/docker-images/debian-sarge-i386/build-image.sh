#!/bin/bash

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to build the container image"
        exit 1;
fi

export ROOT_FS=/tmp/rootfs

rm -rf $ROOT_FS

#sudo debootstrap --arch=i386 sarge $rootfs http://archive.debian.org/debian/
debootstrap --arch=i386 sarge $ROOT_FS http://archive.debian.org/debian/

rm -rf $ROOT_FS/{dev,proc}
mkdir -p $ROOT_FS/{dev,proc}

# provide sane DNS
echo "nameserver 8.8.8.8" > $ROOT_FS/etc/resolv.conf
echo "nameserver 8.8.4.4" >> $ROOT_FS/etc/resolv.conf

# create root fs tarball
tar --numeric-owner -caf rootfs.tar.xz -C $ROOT_FS --transform='s,^./,,' .

docker build -t debian-sarge-i386 .

rm rootfs.tar.xz
rm -rf $ROOT_FS