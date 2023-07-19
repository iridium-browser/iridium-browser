#!/bin/bash

set -o xtrace -o errexit

# These get temporary installed for building Linux and then force-removed.
LINUX_DEV_PKGS="
	bc
	bison
	flex
	libelf-dev
"

# These get temporary installed for building Mesa and then force-removed.
MESA_DEV_PKGS="
	bison
	flex
	gettext
	libwayland-egl-backend-dev
	libxrandr-dev
	llvm-8-dev
	python-mako
	python3-mako
	wayland-protocols
"

# Needed for running the custom-built mesa
MESA_RUNTIME_PKGS="
	libllvm8
"

echo 'deb http://deb.debian.org/debian buster-backports main' >> /etc/apt/sources.list
apt-get update
apt-get -y --no-install-recommends install \
	autoconf \
	automake \
	build-essential \
	curl \
	doxygen \
	freerdp2-dev \
	git \
	libcairo2-dev \
	libcolord-dev \
	libdbus-1-dev \
	libegl1-mesa-dev \
	libevdev-dev \
	libexpat1-dev \
	libffi-dev \
	libgbm-dev \
	libgdk-pixbuf2.0-dev \
	libgles2-mesa-dev \
	libglu1-mesa-dev \
	libgstreamer1.0-dev \
	libgstreamer-plugins-base1.0-dev \
	libinput-dev \
	libjpeg-dev \
	libjpeg-dev \
	liblcms2-dev \
	libmtdev-dev \
	libpam0g-dev \
	libpango1.0-dev \
	libpipewire-0.2-dev \
	libpixman-1-dev \
	libpng-dev \
	libsystemd-dev \
	libtool \
	libudev-dev \
	libva-dev \
	libvpx-dev \
	libwayland-dev \
	libwebp-dev \
	libx11-dev \
	libx11-xcb-dev \
	libxcb1-dev \
	libxcb-composite0-dev \
	libxcb-xfixes0-dev \
	libxcb-xkb-dev \
	libxcursor-dev \
	libxkbcommon-dev \
	libxml2-dev \
	mesa-common-dev \
	ninja-build \
	pkg-config \
	python3-pip \
	python3-setuptools \
	qemu-system \
	sysvinit-core \
	xwayland \
	$MESA_RUNTIME_PKGS


pip3 install --user git+https://github.com/mesonbuild/meson.git@0.49
export PATH=$HOME/.local/bin:$PATH
# for documentation
pip3 install sphinx==2.1.0 --user
pip3 install breathe==4.13.0.post0 --user
pip3 install sphinx_rtd_theme==0.4.3 --user

apt-get -y --no-install-recommends install $LINUX_DEV_PKGS
git clone --depth=1 --branch=drm-next-2020-06-11-1 https://anongit.freedesktop.org/git/drm/drm.git linux
cd linux
make x86_64_defconfig
make kvmconfig
./scripts/config --enable CONFIG_DRM_VKMS
make oldconfig
make -j8
cd ..
mkdir /weston-virtme
mv linux/arch/x86/boot/bzImage /weston-virtme/bzImage
mv linux/.config /weston-virtme/.config
rm -rf linux

# Link to upstream virtme: https://github.com/amluto/virtme
#
# The reason why we are using a fork here is that it adds a patch to have the
# --script-dir command line option. With that we can run scripts that are in a
# certain folder when virtme starts, which is necessary in our use case.
#
# The upstream also has some commands that could help us to reach the same
# results: --script-sh and --script-exec. Unfornutately they are not completely
# implemented yet, so we had some trouble to use them and it was becoming
# hackery.
#
git clone https://github.com/ezequielgarcia/virtme
cd virtme
git checkout -b snapshot 69e3cb83b3405edc99fcf9611f50012a4f210f78
./setup.py install
cd ..

git clone --branch 1.17.0 --depth=1 https://gitlab.freedesktop.org/wayland/wayland
export MAKEFLAGS="-j4"
cd wayland
git show -s HEAD
mkdir build
cd build
../autogen.sh --disable-documentation
make install
cd ../../

apt-get -y --no-install-recommends install $MESA_DEV_PKGS
git clone --single-branch --branch master --shallow-since='2020-02-15' https://gitlab.freedesktop.org/mesa/mesa.git mesa
cd mesa
git checkout -b snapshot c7617d8908a970124321ce731b43d5996c3c5775
meson build -Dauto_features=disabled \
	-Dgallium-drivers=swrast -Dvulkan-drivers= -Ddri-drivers=
ninja -C build install
cd ..
rm -rf mesa

apt-get -y --autoremove purge $LINUX_DEV_PKGS
apt-get -y --autoremove purge $MESA_DEV_PKGS