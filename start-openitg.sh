#!/bin/sh
# if a new patch directory exists, then move it
if [ -d Data\new-patch ]; then
	mv Data\new-patch Data\patch
fi

# Start OpenITG
./openitg-alpha5 --type=Preferences-cabinet

xset r rate 250 24
