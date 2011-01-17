#!/bin/bash
if [ -f /itgdata/K ]; then
	export __GL_FSAA_MODE=1
fi

if [ -f /itgdata/K ]; then
	PARAM="--type=Preferences-kit"

	# Use 640x480 resolution for the kit if /itgdata V exists.
	if [ -f /itgdata/V ]; then
		PARAM="--type=Preferences-kit-vga"
	fi
elif [ -f /itgdata/S ]; then
	PARAM="--type=Preferences-school"
else
	PARAM="--type=Preferences-cabinet"
fi

echo $PARAM

if [ ! -f /stats/premium ]; then
	echo >> /stats/StepMania.ini
	echo "[Options]" >> /stats/StepMania.ini
	echo "Premium=1" >> /stats/StepMania.ini
	touch /stats/premium
fi

/usr/bin/X11/xsetroot -solid '#005050'
/usr/bin/X11/xsetroot -cursor /itgdata/blank /itgdata/blank

export LD_LIBRARY_PATH=/stats/patch/lib

# /stats/patch/install-permanent-patch.sh

if [ -f /tmp/no-start-game ]
then
	sleep 999d
fi

cd /itgdata

exec /stats/patch/openitg $PARAM
