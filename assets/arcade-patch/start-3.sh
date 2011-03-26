#!/bin/bash
if [ -f /itgdata/K ]; then
	export __GL_FSAA_MODE=1
fi

# If we're on a kit and /itgdata/V exists, force the resolution to 640x480
# using our prefs type. This retains ITGIO's global offset and other settings.
if [ -f /itgdata/K ]; then
	if [ -f /itgdata/V ]; then
		PARAM="--type=Preferences-kit-vga"
	else
		PARAM="--type=Preferences-kit"
	fi
elif [ -f /itgdata/S ]; then
	PARAM="--type=Preferences-school"
else
	PARAM="--type=Preferences-cabinet"
fi

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

if [ -f /tmp/no-start-game ]; then
	sleep 999d
else
	cd /itgdata
	exec /stats/patch/openitg $PARAM
fi
