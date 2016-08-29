#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup and enter the docker environment"
        exit 1;
fi

./docker-build.sh "fedora-24-openitg" "cd build/rpm && ./build.sh" "build/rpm/*.rpm"
mkdir artifacts/fedora-24
mv artifacts/openitg*.rpm artifacts/fedora-24/