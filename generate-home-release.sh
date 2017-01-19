#!/bin/bash

source common.sh

has_command "zip" "zip command not found"
has_file "src/openitg" "where's the openitg binary?"

rm -f home-tmp.zip

HOME_TMP_DIR=/tmp/openitg-home-tmp

mkdir -p $HOME_TMP_DIR

# Copy game content
cp -r assets/d4/* $HOME_TMP_DIR
cp -r assets/game-data/* $HOME_TMP_DIR

# Copy patch content
# mkdir -p $HOME_TMP_DIR/Data/patch/patch
# cp -r assets/patch-data/* $HOME_TMP_DIR/Data/patch/patch
# This overwrites files from d4 and game-data where patch-data have newer files.
# Having a patch.zip in the home release has no benefits and only complicates things.
cp -r assets/patch-data/* $HOME_TMP_DIR

# Remove useless files
rm -f $HOME_TMP_DIR/zip.sh
rm -rf $HOME_TMP_DIR/Cache
rm -rf $HOME_TMP_DIR/Songs
rm -rf $HOME_TMP_DIR/Themes/ps2onpc
rm -rf $HOME_TMP_DIR/Themes/ps2
rm -rf $HOME_TMP_DIR/Data/patch/patch/Cache
rm -rf $HOME_TMP_DIR/Data/patch/patch/Songs

cp {ReleaseNotes.txt,WhoToSue.txt} $HOME_TMP_DIR/
cp src/{openitg,GtkModule.so} $HOME_TMP_DIR/

CWD=`pwd`
(cd $HOME_TMP_DIR && zip -u -r $CWD/home-tmp.zip *)

rm -rf $HOME_TMP_DIR