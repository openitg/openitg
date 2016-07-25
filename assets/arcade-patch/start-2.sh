#!/bin/bash
# TOP=/itgdata/default
export TOP=/stats/patch

# Load changed modules.
umount /proc/bus/usb
rmmod ub
rmmod ehci_hcd
rmmod uhci_hcd
rmmod ohci_hcd
rmmod usbcore

insmod $TOP/modules/usbcore.ko
modprobe uhci_hcd
modprobe ohci_hcd
insmod $TOP/modules/ehci-hcd.ko
insmod $TOP/modules/ub.ko
mount /proc/bus/usb

# Not used, so don't load it.
#insmod $TOP/modules/nvram.ko

# Override the 90% set by default with 85%, to fix some clipping problems.
amixer set Master unmute
amixer set Master 85%
amixer set PCM unmute
amixer set PCM 85%

# Default to standard VGA settings and only use others if necessary.
CONF=XF86Config-cab

if [ -f /itgdata/K ]; then
	if [ ! -f /itgdata/V ]; then
		CONF=XF86Config-kit
	fi
elif [ -f /itgdata/S ]; then
	CONF=XF86Config-school
fi

if [ -f /itgdata/S ]; then
	modprobe usbhid
	modprobe joydev
fi

echo $CONF

if [ -f "/tmp/no-start-x" ]; then
	/bin/sleep 999d
	exit 1
fi

PROG=$TOP/start-3.sh
if [ -f "/tmp/no-start-game" ]; then
	PROG="/bin/sleep 999d"
fi

exec xinit $PROG $ARG -- /usr/bin/X11/X -dpi 100 -nolisten tcp -logfile /tmp/XFree86.log -xf86config $TOP/$CONF
