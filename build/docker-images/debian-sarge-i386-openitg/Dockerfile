FROM jedahan/debian-sarge
RUN echo "deb http://archive.debian.org/debian-backports sarge-backports main contrib non-free" >> /etc/apt/sources.list
RUN apt-get update -y
RUN apt-get install -y build-essential gettext automake1.8 gcc g++ libavcodec-dev libavformat-dev libxt-dev libogg-dev libpng-dev libjpeg-dev libvorbis-dev libusb-dev libglu1-mesa-dev libx11-dev libxrandr-dev liblua50-dev liblualib50-dev nvidia-glx-dev libmad0-dev libasound2-dev git-core automake1.7 autoconf libncurses5-dev
CMD /bin/bash
