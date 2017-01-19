=====================================
Rough instructions for a VS2015 build
=====================================

------------------
Build requirements
------------------
  * MSys / mingw-w32 (https://msys2.github.io/)
  * libpng 1.2.56 (https://sourceforge.net/projects/libpng/files/libpng12/1.2.56/lpng1256.zip/download)
  * ffmpeg 3.0.3 (https://www.ffmpeg.org/releases/ffmpeg-3.0.3.tar.gz)
  * NSIS Installer 2.x (https://nsis.sourceforge.net)

------------------
Build instructions
------------------
1. from the Visual Studio command prompt
    * Unzip libpng at lpng1256/: `unzip lpng1256.zip`
    * `set INCLUDE=%INCLUDE%;Z:\path\to\openitg\src\zlib`
    * from the lpng1256 dir, run: `nmake /f .\scripts\makefile.vcwin32`
2. from the repository root in a mingw-w32 shell (`.\msys2_shell.cmd -mingw32 -full-path`)
    * Install build dependencies: `pacman -S unzip diffutils yasm make mingw32/mingw-w64-i686-gcc`
    * Untar ffmpeg at ffmpeg-3.0.3/ and cd into it: `tar -zxvf ffmpeg-3.0.3.tar.gz; cd ffmpeg-3.0.3`
    * run `./configure --prefix=../ffmpeg-build --enable-shared --toolchain=msvc`, wait forever
    * `make && make install`
    * `cp ../ffmpeg-build/bin/*.dll ../Program`
3. from Visual Studio 2015
    * Open src/OpenITG-vs2015.sln
    * Build the Debug and Release build configurations
4. to build the installer, from cmd or powershell
    * `assets\win32-installer\setup-installer-dir.bat`
    * `cd inst-tmp`
    * `makensis /nocd ..\assets\win32-installer\OpenITG.nsi`
