#!/bin/sh

(
	cd assets/utilities/itg2-util
	aclocal && autoconf && autoheader && automake --add-missing
	./configure
	make
)

echo "done"
