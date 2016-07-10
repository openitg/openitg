#!/bin/bash

OPENITG_ARCADE_DOCKER_IMAGE_NAME="openitg-arcade"
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

has_command docker

# ensure docker image is built
DOCKER_IMAGES="`docker images`"

if [[ $DOCKER_IMAGES != *"$OPENITG_ARCADE_DOCKER_IMAGE_NAME"* ]]; then
    cd docker/openitg-arcade/
    ./build-container.sh
    cd ../..
fi

if [ ! -d "artifacts" ]; then
    mkdir artifacts
fi

# Calulate reasonable number of make threads
HAS_NPROC=1
command -v nproc >/dev/null 2>&1 || { HAS_NPROC=0; }

if [ $HAS_NPROC -eq 1 ] ; then
    MAKE_THREADS=$(nproc)
else
    MAKE_THREADS=1
fi

# Copy private rsa key to a temporary location while building so that we dont have to transform relative paths
cp $PRIVATE_RSA ../docker-temp-private.rsa

# Change directory to the root of the repository
cd ..

if [ -f "src/openitg" ]; then
    rm src/openitg
fi

if [ -f "src/GtkModule.so" ]; then
    rm src/GtkModule.so
fi

# Creates a docker container that:
# 1. Builds an arcade binary in a copy of the repository (no files are changed on the host)
# 2. Copies the openitg binary to the src/ directory on the host
# 3. Throws away the container (--rm)
docker run --rm -v `pwd`:/root/openitg -t openitg-arcade /bin/bash -c "cd /root/openitg/build/arcade-patch && ./inside-container-build.sh $MAKE_THREADS"

# Verify the only output is where it's supposed to be.
has_file "src/openitg" "%s doesn't exist! Did the build fail?"

# Build the itg2-util
cd assets/utilities/itg2-util
./autogen.sh
./configure
make
cd ../../..

# Generate a machine revision package
./gen-arcade-patch.sh docker-temp-private.rsa
rm docker-temp-private.rsa

mv ITG*.itg build/artifacts/
cd build
