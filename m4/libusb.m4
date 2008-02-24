AC_DEFUN([SM_LIBUSB],
[
	AC_CHECK_LIB(usb, usb_init, have_libusb=yes, have_libusb=no)
	AC_CHECK_HEADER(usb.h, have_libusb_header=yes, have_libusb_header=no)

	if test "$have_libusb_header" = "no"; then
		have_libusb="no"
	fi

	if test "$have_libusb" = "yes"; then
	        AC_DEFINE(HAVE_LIBUSB, 1, [libusb support])
		AM_CONDITIONAL(HAVE_LIBUSB, 1)
		LIBS="$LIBS -lusb"
	else
		AC_DEFINE(HAVE_LIBUSB, 0, [libusb support])
		AM_CONDITIONAL(HAVE_LIBUSB, 0)
	fi
])
