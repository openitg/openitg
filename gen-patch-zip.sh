#!/bin/bash

# Usage: gen-patch-zip [out-file]
#
# Generates an encrypted patch ZIP for all releases. If out-file is
# specified, the encrypted patch ZIP is output to that file. Otherwise, the
# patch ZIP is output to ./patch.zip.
# -- vyhd

set -e

# include convenience functions and constants
source common.sh

# the path containing the game data to be used for the patch
PATCH_DIR="$ASSETS_DIR/patch-data"

# the patch file containing our data, using ./patch.zip by default.
# if we have an argument, zip and encrypt into that file instead.
PATCH_FILE=${1:-patch.zip}

# does in-place encryption (well, using a temp file)
function encrypt-patch {
	TMP_FILE="/tmp/oitg-patch-enc-tmp"

	if [ -z $1 ]; then
		echo "you didn't give me anything to encrypt!"
		return 1
	fi

	# leave this visible in case it throws an error
	$ITG2_UTIL_PATH -p "$1" "$TMP_FILE"

	mv "$TMP_FILE" "$1"
}

# Require that the encryption file actually exists
has_file "$ITG2_UTIL_PATH" "$ITG2_UTIL_FILE must be built in $ITG2_UTIL_DIR first."

# delete the patch file if it exists, or zip will try to update it
rm -f "$PATCH_FILE"

# we need this so we can put patch.zip in the CWD
CWD=`pwd`

# mild hack: wipe that CWD if we're using an absolute path
if [ ${PATCH_FILE:0:1} == "/" ]; then CWD=""; fi

pushd "$PATCH_DIR"
echo "Zipping files into $PATCH_FILE..."
zip -rq "$CWD/$PATCH_FILE" ./*/
popd

echo "Encrypting patch data..."
encrypt-patch "$PATCH_FILE"

echo "Patch successfully created: $PATCH_FILE"
