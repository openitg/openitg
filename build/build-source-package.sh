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

START_DIR=`pwd`

OPENITG_VERSION="`git describe --abbrev=0`"

TEMP_WORK_DIR="/tmp/openitg-work-tmp"

if [ -d $TEMP_WORK_DIR ]; then
    rm -rf $TEMP_WORK_DIR
fi

mkdir -p $TEMP_WORK_DIR/openitg-$OPENITG_VERSION
cp -r ../* $TEMP_WORK_DIR/openitg-$OPENITG_VERSION
cd $TEMP_WORK_DIR/openitg-$OPENITG_VERSION

# When downloading a source package it should be prepared to run configure
./autogen.sh

cd ..

tar -zcf $TEMP_WORK_DIR/openitg-$OPENITG_VERSION.tar.gz openitg-$OPENITG_VERSION

cd $START_DIR
mv $TEMP_WORK_DIR/openitg-$OPENITG_VERSION.tar.gz $OUTPUT_DIR/

rm -rf $TEMP_WORK_DIR