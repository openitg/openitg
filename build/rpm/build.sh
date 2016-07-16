#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

mkdir -p /usr/src/packages/SOURCES
(cd .. && ./build-source-package.sh /usr/src/packages/SOURCES)

rpmbuild -bb openitg.spec

# When this script finishes the container is destroyed. The only thing left standing is the mounted repository.
# Any file that should be preserved must be in the repository.
cp /usr/src/packages/RPMS/x86_64/* .
