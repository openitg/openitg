=========================================
Barebones instructions for a VS2015 build
=========================================

Build requirements:
  * MSys / mingw-w64 (https://msys2.github.io/)
  * libpng 1.2.56 (https://sourceforge.net/projects/libpng/files/libpng12/1.2.56/lpng1256.zip/download)
  * ffmpeg 3.0.3 (https://www.ffmpeg.org/releases/ffmpeg-3.0.3.tar.gz)
  * NSIS Installer 2.x (https://nsis.sourceforge.net)

Build instructions (starting from the repository root in a mingw-w64 shell):
  * Unzip libpng at libpng1256/: `unzip lpng1256.zip`
  * Untar ffmpeg at ffmpeg-3.0.3/ and cd into it: `tar -zxvf ffmpeg-3.0.3.tar.gz; cd ffmpeg-3.0.3`
  * run `./configure --prefix=../ffmpeg-build`, wait forever
