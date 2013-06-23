#!/bin/bash

# /openitg/util/itg2-patch-encrypt
# when it's committed. until then, sorry
# ENCRYPTER="../../../util/itg2-patch-encrypt"

ENCRYPTER="itg2-patch-encrypt"

DIRS="Cache Data NoteSkins Songs Themes"
NO_SONG_DIRS="Data NoteSkins Themes"

delete()
{
	if [ -f "$1" ]; then rm "$1"; fi
}

delete patch-dec.zip
delete patch.zip
delete patch-no-songs-dec.zip
delete patch-no-songs.zip

# ignore all Subversion data
zip -r patch-dec.zip $DIRS -x \*/.svn/\*
zip -r patch-no-songs-dec.zip $NO_SONG_DIRS -x \*/.svn/\*

$ENCRYPTER patch-dec.zip patch.zip
$ENCRYPTER patch-no-songs-dec.zip patch-no-songs.zip

exit 0
