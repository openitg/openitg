#!/bin/bash

PRIVATE_RSA="$1"

# exit immediately on nonzero exit code
set -e
set -u

source ../common.sh

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup and enter the docker environment"
        exit 1;
fi

function print_usage
{
	echo "Usage: $0 [private RSA key]"
	exit 0
}

if ! [ -f "$PRIVATE_RSA" ]; then print_usage; fi

PRIVATE_RSA=$(abspath $PRIVATE_RSA)

# Calulate reasonable number of make jobs
HAS_NPROC=1
command -v nproc >/dev/null 2>&1 || { HAS_NPROC=0; }

if [ $HAS_NPROC -eq 1 ] ; then
    MAKE_JOBS=$(nproc)
else
    MAKE_JOBS=1
fi

./docker-build.sh "openitg-arcade" "./build-arcade.sh $MAKE_JOBS" "src/openitg"

# Change directory to the root of the repository
cd ..

if [ -f "src/openitg" ]; then
    rm src/openitg
fi

if [ -f "src/GtkModule.so" ]; then
    rm src/GtkModule.so
fi

mv build/artifacts/openitg src/

# Verify the only output is where it's supposed to be.
has_file "src/openitg" "%s doesn't exist! Did the build fail?"

# Build the itg2-util
cd assets/utilities/itg2-util
./autogen.sh
./configure
make
cd ../../..

# Generate a machine revision package
./gen-arcade-patch.sh $PRIVATE_RSA

mv ITG*.itg build/artifacts/
cd build
