AC_DEFUN([SM_LIBUSB],
[
	AC_CHECK_LIB(usb, usb_init, have_libusb=yes, have_libusb=no)
	AC_CHECK_HEADER(usb.h, have_libusb_header=yes, have_libusb_header=no)

	if test "$have_libusb_header" = "no" -o "$have_libusb" = "no"; then
		echo " *** libusb not found on system (you DO want PIUIO...right?) ***"
		echo " *** please install/compile libusb and re-configure          ***"
		exit 1
	fi

	LIBS="$LIBS -lusb"
])
