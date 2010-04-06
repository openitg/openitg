#!/bin/sh
# if a new patch directory exists, then move it
if [ -d Data/new-patch ]; then
	mv Data/new-patch Data/patch
	mv openitg openitg.old
	mv Data/patch/openitg openitg
	chmod +x openitg
fi

# Start OpenITG
./openitg --type=Preferences-cabinet

xset r rate 250 24
