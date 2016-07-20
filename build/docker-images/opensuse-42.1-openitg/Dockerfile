FROM opensuse:42.1
RUN zypper -n install rpm-build
RUN zypper -n install zip unzip git gcc-c++ automake autoconf
RUN zypper -n install Mesa-devel alsa-devel libpng12-compat-devel glu-devel libjpeg-devel
RUN zypper -n install libXrandr-devel libusb-compat-devel ffmpeg-devel libvorbis-devel libogg-devel gtk2-devel lua51-devel
CMD /bin/bash
