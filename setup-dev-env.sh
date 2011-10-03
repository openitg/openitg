#!/bin/bash

RUN_DIR=run

mkdir -p $RUN_DIR

for i in `cd assets/d4 && ls`; do
    cp -rl assets/d4/$i $RUN_DIR/$i;
done

for i in `cd assets/game-data && ls`; do
    cp -rlf assets/game-data/$i $RUN_DIR;
done

ln -s ../src/openitg $RUN_DIR/openitg
ln -s ../src/GtkModule.so $RUN_DIR/GtkModule.so

mkdir $RUN_DIR/Data/patch
ln -s ../../../assets/patch-data $RUN_DIR/Data/patch/patch
