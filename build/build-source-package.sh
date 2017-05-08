#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

OUTPUT_DIR="$1"

function print_usage
{
	echo "Usage: $0 [output directory]"
	exit 0
}

if ! [ -d "$OUTPUT_DIR" ]; then print_usage; fi

pushd .

OPENITG_VERSION="`git describe | sed -r 's/-/+/g'`"

TEMP_WORK_DIR="/tmp/openitg-work-tmp"

if [ -d $TEMP_WORK_DIR ]; then
    rm -rf $TEMP_WORK_DIR
fi

mkdir -p $TEMP_WORK_DIR
cp -r .. $TEMP_WORK_DIR/openitg-$OPENITG_VERSION

# Enter repository
cd $TEMP_WORK_DIR/openitg-$OPENITG_VERSION

# Clean all changes, added files, and files hidden by .gitignore
#git clean -dfx

# Remove git files
#rm -rf .git .gitignore

# When a user downloads a source package it should be prepared for running ./configure
./autogen.sh

cd ..

tar --exclude='.git*' -Jcf $TEMP_WORK_DIR/openitg-$OPENITG_VERSION.tar.xz openitg-$OPENITG_VERSION

popd
mv $TEMP_WORK_DIR/openitg-$OPENITG_VERSION.tar.xz $OUTPUT_DIR/

rm -rf $TEMP_WORK_DIR
