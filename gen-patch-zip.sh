#!/bin/bash

set -e
set -u

# the data containing all the patch data to be zipped (relative to CWD)
PATCH_DIR="assets/patch-data"

# the patch file containing the above (relative to CWD)
PATCH_FILE="patch.zip"

# utility file to be used for encrypting the patch
ENCRYPT_DIR="assets/utilities/itg2-util/src"
ENCRYPT_FILE="itg2ac-util"	# hurr hurr hurr
ENCRYPTER="$ENCRYPT_DIR/$ENCRYPT_FILE"

if [ ! -f "$ENCRYPTER" ]; then
	echo "$ENCRYPT_FILE must be built before using this script!"
	echo "please cd on over to $ENCRYPT_DIR and build it."
	exit 1
fi

# does in-place encryption (well, using a temp file)
function encrypt-patch {
	TMP_FILE="/tmp/oitg-patch-enc-tmp"

	if [ -z $1 ]; then
		echo "you didn't give me anything to encrypt!"
		return 1
	fi

	# leave this visible in case it throws an error
	"$ENCRYPTER" -p "$1" "$TMP_FILE"

	mv "$TMP_FILE" "$1"
}

# delete the patch file if it exists, or zip
# complains about an invalid structure.
if [ -f "$PATCH_FILE" ]; then rm "$PATCH_FILE"; fi

# we need this so we can put patch.zip in the CWD
CWD=`pwd`

cd "$PATCH_DIR"
echo "Zipping files into $PATCH_FILE..."
zip -rq "$CWD/$PATCH_FILE" ./*/
cd - 0&> /dev/null

echo "Encrypting patch data..."
encrypt-patch "$PATCH_FILE"

echo "Patch successfully created: $PATCH_FILE"
