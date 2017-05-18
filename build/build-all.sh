#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

PRIVATE_RSA="$1"

function print_usage
{
	echo "Usage: $0 [private RSA key]"
	exit 0
}

if ! [ -f "$PRIVATE_RSA" ]; then print_usage; fi

if [ ! -d "artifacts" ]; then
    mkdir artifacts
fi

./build-source-package.sh ./artifacts
./build-arcade-machine-revision.sh $PRIVATE_RSA
./build-opensuse-package.sh
./build-ubuntu-packages.sh
