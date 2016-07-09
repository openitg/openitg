FROM scratch
ADD rootfs.tar.xz /
RUN echo "deb http://archive.debian.org/debian-backports sarge-backports main contrib non-free" >> /etc/apt/sources.list
RUN apt-get update -y
CMD /bin/bash
