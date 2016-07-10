#!/bin/sh

./autogen.sh
./configure --with-x --with-gnu-ld --enable-itg-arcade --with-legacy-ffmpeg --target=i386-pc-linux-gnu --host=i386-pc-linux-gnu
make clean
if [ "x$1" == "x" ];
then
	make
else
	make -j$1
fi
