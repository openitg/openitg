FROM fedora:24
RUN dnf -y install @development-tools fedora-packager rpmdevtools
RUN dnf -y install zip unzip git gcc-c++ automake autoconf
RUN dnf -y install mesa-libGLU-devel alsa-lib-devel libpng12-devel libjpeg-turbo-devel gettext-devel
RUN dnf -y install libXrandr-devel libusb-devel libvorbis-devel libogg-devel gtk2-devel lua-devel
CMD /bin/bash
