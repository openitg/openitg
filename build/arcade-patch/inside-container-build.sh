#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

cd /root
cp -r openitg/ openitg-build/

cd openitg-build

if [ "x$1" == "x" ];
then
	./build-arcade.sh
else
    ./build-arcade.sh $1
fi

cp src/openitg /root/openitg/src/
#cp src/GtkModule.so /root/openitg/src/

cd /root
rm -rf openitg-build/