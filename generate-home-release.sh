#!/bin/sh

rm -f home-tmp.zip
if [ "`which zip`x" = "x" ]; then
	echo "zip command not found"
	exit
fi
(cd assets/game-data && zip -u -r ../../home-tmp.zip *)
./configure --with-gnu-ld --with-x
make || (echo "Could not build OpenITG executable.  exiting..."; exit 1)

(cd src; zip -u ../home-tmp.zip openitg)
