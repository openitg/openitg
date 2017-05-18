# OpenITG

An open-source rhythm dancing game which is a fork of StepMania 3.95
with the goal of adding arcade-like ITG-style behavior and serving as a drop-in
replacement for the ITG binary on arcade cabinents.

* Project homepage: https://github.com/openitg/openitg/wiki
* Project bug tracker: https://github.com/openitg/openitg/issues
* Project IRC channel: #openitg on irc.badnik.com
* Project source code: https://github.com/openitg/openitg

## TODOs

### Short-term

1. Getting Started Guide - build and development
2. self-contained cache-rebuilding solution
3. OpenGL Driver uses fix function pipeline rather than shader

### Long-term

1. StepMania 4.0 LUA Bindings
2. StepMania 4.0 Theme metrics


## How to check-out the source

```sh
git clone https://github.com/openitg/openitg.git
```

## How to contribute

1. Create an account at github.com
2. Goto https://github.com/openitg/openitg
3. Click "fork"
4. `git clone https://github.com/<username>/openitg.git`
5. Edit files...
6. `git add <filename>` for every file you add or edit
7. `git commit` # now your change is committed locally
8. `git push` # now your change is pushed to your github
9. From https://github.com/<username>/openitg, click "pull request".  Base branch is the
branch you want to put your changes on, and head branch is the branch you made
your changes to already.
10. Write a short description of your change.  Be sure to include the goal, any
bugs fixed, features added, etc, and any credit you wish to have.  Click "send
pull request".

## How to build for arcade

1. Choose a location for your chroot:  MY_CHROOT=/home/cmyers/chroot
2. Install debootstrap and chroot (on debian/ubuntu, apt-get install chroot debootstrap)
3. Set up chroot, from root dir of source, as the root user, run: ./chroot-arcade.sh `pwd` $MY_CHROOT
4. cd /root/openitg-dev/ && ./build-arcade.sh

NOTE: the chroot will be created in the location you choose for MY_CHROOT.  This
will build an entire Debian Sarge Linux system (the same OS used by arcade
machines).  This will take approximately 350MB.  A full clone of the repo is
about 300MB after you build all artifacts, so expect to have at least 650MB of
free space to work with.

## How to build for home on 32-bit linux:

TODO: No chroot necessary, need script to install dependencies on various
distributions...

## How to build for home on 64-bit linux:

TODO: Similar process to arcade, but create 32-bit chroot of modern debian

## How to build for home on windows:

1. Open visual studio (2010, 2013, and 2015 have been tested)
2. Select "File -> Open Project" and pick src\Stepmania-net2010.sln
3. 5 Projects will load in the solution explorer. Right click each and select properties.
4. For each of the project properties, under the configurations drop-down, select "All Configurations" and under Configuration "Options->General" select the appropriate platform toolset for your VS Version.
5. If you use Visual Studio 2013, depending on your edition of it, there is a bug, you will need to add the /FS flag to Additional Options under "C/C++ -> Command Line", or you can simply build twice the first time you build.
6. To compile the release, select the appropriate profile (usually the SSE2 build), then select "Build -> Rebuild Solution".

