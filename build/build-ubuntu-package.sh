#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to setup and enter the docker environment"
        exit 1;
fi

./docker-build.sh "ubuntu-openitg" "cd build/deb && ./build.sh" "build/deb/*.deb"
