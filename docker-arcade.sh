#!/bin/sh

docker run -v `pwd`:/root/oitg -t $USER/oitg /bin/bash -c 'cd /root/oitg && ./build-arcade.sh'
