# Simple guide
# https://fedoraproject.org/wiki/How_to_create_an_RPM_package
# Reference
# https://docs.fedoraproject.org/en-US/Fedora_Draft_Documentation/0.1/html/RPM_Guide/index.html

# Naming av versioning
# https://en.opensuse.org/openSUSE:Package_naming_guidelines

%if %{undefined dist}
    %if 0%{?suse_version} == 1315
        %define mydist .suse42.1
    %endif
%endif

Summary: An open-source rhythm dancing game based on StepMania 3.95
Name: openitg
Version: %(git describe | sed -r 's/-/+/g')
# https://fedoraproject.org/wiki/Packaging:DistTag?rd=Packaging/DistTag
Release: 1%{?dist}%{?mydist}
License: MIT
Group: Games
Source0: openitg-%{version}.tar.xz
URL: https://github.com/openitg/openitg
Distribution: SUSE Linux
Vendor: -
Packager: August Gustavsson

# https://en.opensuse.org/openSUSE:Build_Service_cross_distribution_howto#Detect_a_distribution_flavor_for_special_code
%if 0%{?suse_version}
BuildRequires: zip unzip git gcc-c++ automake autoconf make
BuildRequires: Mesa-devel alsa-devel libpng12-compat-devel glu-devel libjpeg-devel
BuildRequires: libXrandr-devel libusb-compat-devel libvorbis-devel libogg-devel gtk2-devel lua51-devel
BuildRequires: libavcodec-devel libavformat-devel libavutil-devel libswscale-devel
Requires: libpng12-0
%endif

%if 0%{?fedora}
BuildRequires: zip unzip git gcc automake autoconf make
BuildRequires: mesa-libGLU-devel alsa-lib-devel libpng12-devel libjpeg-turbo-devel gettext-devel
BuildRequires: libXrandr-devel libusb-devel libvorbis-devel libogg-devel gtk2-devel lua-devel
%endif

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
make %{?_smp_mflags}
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
