Summary: An open-source rhythm dancing game based on StepMania 3.95
Name: openitg
Version: %(git describe --abbrev=0)
Release: 1
License: MIT
Group: Games
Source0: openitg-%{version}.tar.gz
URL: https://github.com/openitg/openitg
Distribution: SUSE Linux
Vendor: -
Packager: August Gustavsson
BuildRequires: zip unzip git gcc-c++ automake autoconf make
BuildRequires: Mesa-devel alsa-devel libpng12-compat-devel glu-devel libjpeg-devel
BuildRequires: libXrandr-devel libusb-compat-devel libvorbis-devel libogg-devel gtk2-devel lua51-devel
BuildRequires: libavcodec-devel libavformat-devel libavutil-devel libswscale-devel
Requires: libpng12-0

%description
An open-source rhythm dancing game which is a fork of StepMania 3.95
with the goal of adding arcade-like ITG-style behavior and serving as a drop-in
replacement for the ITG binary on arcade cabinents.

%prep
%setup

%build
./autogen.sh
%configure --with-sse2 --with-x --with-gnu-ld --without-mp3
make clean
make -j4
strip --strip-unneeded src/openitg

# Creates home-tmp.zip
./generate-home-release.sh


%install
mkdir -p %{buildroot}/opt/%{name}
#install -D -m 0755 src/openitg %{buildroot}/opt/%{name}/%{name}
#install -D -m 0755 src/GtkModule.so %{buildroot}/opt/%{name}/GtkModule.so
unzip home-tmp.zip -d %{buildroot}/opt/%{name}/
rm home-tmp.zip
chmod -R 664 %{buildroot}/opt/%{name}/*
chmod 755 %{buildroot}/opt/%{name}/%{name}
chmod 755 %{buildroot}/opt/%{name}/GtkModule.so

mkdir -p %{buildroot}%{_bindir}
cat <<EOF > %{buildroot}%{_bindir}/%{name}
#!/bin/sh
exec /opt/%{name}/%{name}
EOF

chmod 755 %{buildroot}%{_bindir}/%{name}

%files
%defattr(-,root,root)
%dir /opt/%{name}
/opt/%{name}/*
%{_bindir}/%{name}

%changelog
