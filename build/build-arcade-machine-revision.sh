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

# Calculate reasonable number of make jobs
HAS_NPROC=1
command -v nproc >/dev/null 2>&1 || { HAS_NPROC=0; }

if [ $HAS_NPROC -eq 1 ] ; then
    MAKE_JOBS=$(nproc)
else
    MAKE_JOBS=1
fi

./docker-build.sh "debian-sarge-i386-openitg" "./build-arcade.sh $MAKE_JOBS" "src/openitg"

# Change directory to the root of the repository
cd ..

if [ -f "src/openitg" ]; then
    rm src/openitg
fi

if [ -f "src/GtkModule.so" ]; then
    rm src/GtkModule.so
fi

mv build/artifacts/openitg src/

# Build itg2-util
pushd assets/utilities/itg2-util
./autogen.sh
./configure
make
popd

# Generate a machine revision package
./gen-arcade-patch.sh $PRIVATE_RSA

mv ITG*.itg build/artifacts/
cd build
