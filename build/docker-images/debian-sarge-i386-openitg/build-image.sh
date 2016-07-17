#!/bin/bash

if [ ! "x`whoami`" = "xroot" ]; then
        echo "$0: you must be root to build the container image"
        exit 1;
fi

docker build -t debian-sarge-i386-openitg .
