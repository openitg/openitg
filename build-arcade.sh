#!/bin/sh

# XXX: Move this to make land!
./generate-version

./autogen.sh
./configure --with-x --with-gnu-ld --enable-itg-arcade --target=i386-pc-linux-gnu --host=i386-pc-linux-gnu
make
