#!/bin/sh

./autogen.sh
./configure --with-x --with-gnu-ld --enable-itg-arcade --target=i386-pc-linux-gnu --host=i386-pc-linux-gnu
make


