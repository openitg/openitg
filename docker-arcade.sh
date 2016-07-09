#!/bin/sh

docker run -v `pwd`:/root/openitg -t openitg-arcade /bin/bash -c 'cd /root/openitg && ./build-arcade.sh'
