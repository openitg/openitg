#!/bin/bash

# Copies the openitg repository to a docker container and runs a specified build script.
#
# Only the artifacts are copied back to the original repository.
# Operation:
# 1. Makes sure the desired docker image is built. The image name corresponds to a 
#    directory in the docker-images directory.
# 2. Copy the entire openitg repository to a location in /tmp
# 3. Start a docker container and mount the repo copy at /root/openitg in the container
# 4. cd to the root of the repository inside the container
# 5. Run the specified "build-command"
# 6. Throw away the container
# 7. Copy the specified artifacts to build/artifacts in the original repo
# 8. Delete the repository copy under /tmp 

# exit immediately on nonzero exit code
set -e
set -u

source ../common.sh

DOCKER_IMAGE_NAME="$1"
BUILD_COMMAND="$2"
ARTIFACTS="$3"

function print_usage
{
	echo "Usage: $0 [docker-image] [build-command] [artifacts]"
	exit 0
}

if ! [ -d "docker-images/$DOCKER_IMAGE_NAME" ]; then print_usage; fi
if [ -z "$BUILD_COMMAND" ]; then print_usage; fi
if [ -z "$ARTIFACTS" ]; then print_usage; fi

has_command "docker"

if [ ! -d "artifacts" ]; then
    mkdir artifacts
fi

# ensure docker image is built
DOCKER_IMAGES="`docker images`"

if [[ $DOCKER_IMAGES != *"$DOCKER_IMAGE_NAME"* ]]; then
    (cd docker-images/$DOCKER_IMAGE_NAME && ./build-image.sh)
fi

# Change directory to the root of the repository
cd ..

# Creates a docker container that:
# 1. Builds an artifact in a copy of the repository (no files are changed on the host)
# 2. Copies the specified artifacts to the artifacts/ directory on the host
# 3. Throws away the container (--rm)
OPENITG_SRC_DOCKER="/tmp/openitg-src-docker"
rm -rf $OPENITG_SRC_DOCKER
cp -r . $OPENITG_SRC_DOCKER
rm -rf $OPENITG_SRC_DOCKER/run

docker run --rm -v $OPENITG_SRC_DOCKER:/root/openitg -it $DOCKER_IMAGE_NAME /bin/bash -c "cd /root/openitg && $BUILD_COMMAND"

ORIGINAL_REPO=`pwd`
(cd $OPENITG_SRC_DOCKER && cp $ARTIFACTS $ORIGINAL_REPO/build/artifacts/)

rm -rf $OPENITG_SRC_DOCKER

cd build
