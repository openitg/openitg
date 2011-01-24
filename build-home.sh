#!/bin/sh

./autogen.sh
# TODO: make this actually build 64-bit
./configure --with-x --with-gnu-ld --target=i386-pc-linux-gnu --host=i386-pc-linux-gnu
make


