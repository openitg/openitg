#!/bin/bash

HOST=root@itg
if [[ "x$1" != "x" ]]; then
    HOST=$1
fi

PORT=22
if [[ "x$2" != "x" ]]; then
  PORT=$2
fi

scp -P $PORT src/openitg $HOST:/stats/patch/openitg-`git describe`
ssh -p $PORT $HOST "mv /stats/patch/openitg /stats/patch/openitg.old && ln -s /stats/patch/openitg-`git describe` /stats/patch/openitg"
ssh -p $PORT $HOST 'pkill openitg'

