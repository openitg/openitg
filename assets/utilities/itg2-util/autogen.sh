#!/bin/sh

aclocal
autoconf
autoheader
automake --add-missing
./configure --build=i386-pc-linux-gnu
