#!/bin/sh

if [ "`which javac`x" = "x" ]; then
	echo "javac missing"
	exit 1
fi
if [ "`which java`x" = "x" ]; then
	echo "java missing"
	exit 1
fi
if [ ! -f src/openitg ]; then
	echo "where's the OpenITG binary?"
	exit 1
fi

# arcade patch.zip utility
if [ ! -f assets/utilities/itg2-util/src/itg2ac-util ]; then
	(
		cd assets/utilities/itg2-util
		aclocal && autoconf && autoheader && automake --add-missing
		./configure
		make
	)
fi

# machine revision signer
if [ ! -f src/verify_signature/java/SignFile.class ]; then
	(
		cd src/verify_signature/java
		javac SignFile.java
	)
fi

# generate patch.zip file
rm -f assets/patch-data/patch-dec.zip assets/patch-data/patch.zip
(
	cd assets/patch-data/patch-dec
	zip -0 -r ../patch-dec.zip *
	../../utilities/itg2-util/src/itg2ac-util -p ../patch-dec.zip ../patch.zip	
)

# move everything into a zip file
rm -f openitg-tmp.itg
(
	cd assets/patch-data
	zip -r ../../openitg-tmp.itg * -x 'patch-dec/*' patch-dec.zip
)

# ..including the binary
(
	cd src
	zip ../../openitg-tmp.itg openitg
)

# sign .itg file
java -classpath src/verify_signature/java SignFile openitg-tmp.itg OpenITG-Private.rsa openitg-tmp.sig
cat openitg-tmp.sig >>openitg-tmp.itg
