#!/bin/sh

./autogen.sh
./configure --with-x --with-gnu-ld --enable-itg-arcade --target=i386-pc-linux-gnu --host=i386-pc-linux-gnu
if [ "x$1" == "x" ];
then
	make
else
	make -j$1
fi
