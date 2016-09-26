=========================================
Barebones instructions for a VS2015 build
=========================================

Build requirements:
  * MSys / mingw-w32 (https://msys2.github.io/)
  * libpng 1.2.56 (https://sourceforge.net/projects/libpng/files/libpng12/1.2.56/lpng1256.zip/download)
  * ffmpeg 3.0.3 (https://www.ffmpeg.org/releases/ffmpeg-3.0.3.tar.gz)
  * NSIS Installer 2.x (https://nsis.sourceforge.net)

Build instructions (starting from the repository root in a mingw-w32 shell):
  * Unzip libpng at libpng1256/: `unzip lpng1256.zip`
  * Untar ffmpeg at ffmpeg-3.0.3/ and cd into it: `tar -zxvf ffmpeg-3.0.3.tar.gz; cd ffmpeg-3.0.3`
  * run `./configure --prefix=../ffmpeg-build --enable-shared --toolchain=msvc`, wait forever
  * `make && make install`
  * Open src/OpenITG-vs2015.sln in Visual Studio 2015
  * Build the Debug and Release build configurations
  * to build the installer, run the following from the repository root in cmd or powershell:
    * `assets\win32-installer\setup-installer-dir.bat`
    * `cd inst-tmp`
    * `makensis /nocd ..\assets\win32-installer\OpenITG.nsi`
