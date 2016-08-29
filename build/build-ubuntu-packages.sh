#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup and enter the docker environment"
        exit 1;
fi

./docker-build.sh "ubuntu-15.10-openitg" "cd build/deb && ./build.sh" "build/deb/*.deb"
mkdir artifacts/ubuntu-15.10
mv artifacts/openitg*.deb artifacts/ubuntu-15.10/

./docker-build.sh "ubuntu-16.04-openitg" "cd build/deb && ./build.sh" "build/deb/*.deb"
mkdir artifacts/ubuntu-16.04
mv artifacts/openitg*.deb artifacts/ubuntu-16.04/