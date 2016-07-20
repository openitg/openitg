#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

# OpenSUSE wants the source in /usr, fedora wants it in /root. Cover both, the container will get thrown away anyway.
mkdir -p /usr/src/packages/SOURCES
mkdir -p /root/rpmbuild/SOURCES

(cd .. && ./build-source-package.sh /usr/src/packages/SOURCES)
cp /usr/src/packages/SOURCES/* /root/rpmbuild/SOURCES/

rpmbuild -bb openitg.spec

# When this script finishes the container is destroyed. The only thing left standing is the mounted repository.
# Any file that should be preserved must be in the repository.
cp /usr/src/packages/RPMS/x86_64/* .
#cp /root/rpmbuild/RPMS/x86_64/* .