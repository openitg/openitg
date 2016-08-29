#!/bin/bash

# exit immediately on nonzero exit code
set -e
set -u

DEB_DIR=`pwd`

# cd to the root of the repository
cd ../../

# build home release
./build-home.sh 4
./generate-home-release.sh
mv home-tmp.zip $DEB_DIR/

cd $DEB_DIR

# Build debian package
# http://askubuntu.com/questions/1345/what-is-the-simplest-debian-packaging-guide
# https://ubuntuforums.org/showthread.php?t=910717

PACKAGE_NAME=openitg
# deb package version numbers must start with a digit...
PACKAGE_VERSION=`git describe | sed -r 's/-/+/g'`
PACKAGE_RELEASE="1"
PACKAGE_ARCH="amd64"
PACKAGE_DEB_VERSION="$PACKAGE_VERSION-$PACKAGE_RELEASE"
PACKAGE_FULLNAME=${PACKAGE_NAME}_${PACKAGE_DEB_VERSION}_${PACKAGE_ARCH}

rm -rf $PACKAGE_FULLNAME
mkdir $PACKAGE_FULLNAME
mkdir -p $PACKAGE_FULLNAME/DEBIAN
mkdir -p $PACKAGE_FULLNAME/opt/$PACKAGE_NAME
mkdir -p $PACKAGE_FULLNAME/usr/bin

unzip home-tmp.zip -d $PACKAGE_FULLNAME/opt/$PACKAGE_NAME/
rm home-tmp.zip
chmod -R 664 $PACKAGE_FULLNAME/opt/$PACKAGE_NAME/*
chmod 755 $PACKAGE_FULLNAME/opt/$PACKAGE_NAME/$PACKAGE_NAME
chmod 755 $PACKAGE_FULLNAME/opt/$PACKAGE_NAME/GtkModule.so

cat <<EOF > $PACKAGE_FULLNAME/usr/bin/$PACKAGE_NAME
#!/bin/sh
exec /opt/$PACKAGE_NAME/$PACKAGE_NAME
EOF

chmod 755 $PACKAGE_FULLNAME/usr/bin/$PACKAGE_NAME

cat <<EOF > $PACKAGE_FULLNAME/DEBIAN/control
Package: $PACKAGE_NAME
Version: $PACKAGE_DEB_VERSION
Section: games
Priority: optional
Architecture: $PACKAGE_ARCH
Depends: libglu1, libpng12-0, libmad0, libogg0, libvorbis0a, ffmpeg (>= 7:2.7), liblua50, liblualib50, libxrandr2, libxt6
Maintainer: August Gustavsson
Description: An open-source rhythm dancing game which is a fork of StepMania 3.95
 with the goal of adding arcade-like ITG-style behavior and serving as a drop-in
 replacement for the ITG binary on arcade cabinents.
EOF

dpkg-deb --build $PACKAGE_FULLNAME
