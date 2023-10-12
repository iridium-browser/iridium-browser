#!/bin/bash
#
# Constructs the base container image used to build Weston within CI. Per the
# comment at the top of .gitlab-ci.yml, any changes in this file must bump the
# $FDO_DISTRIBUTION_TAG variable so we know the container has to be rebuilt.

set -o xtrace -o errexit

# These get temporary installed for building Linux and then force-removed.
LINUX_DEV_PKGS="
	bc
	bison
	flex
"

# These get temporary installed for building Mesa and then force-removed.
MESA_DEV_PKGS="
	bison
	flex
	gettext
	libwayland-egl-backend-dev
	libxrandr-dev
	libxshmfence-dev
	libxrandr-dev
	llvm-11-dev
	python3-mako
"

# Needed for running the custom-built mesa
MESA_RUNTIME_PKGS="
	libllvm11
"

apt-get update
apt-get -y --no-install-recommends install \
	autoconf \
	automake \
	build-essential \
	clang-11 \
	curl \
	doxygen \
	graphviz \
	freerdp2-dev \
	gcovr \
	git \
	hwdata \
	lcov \
	libasound2-dev \
	libbluetooth-dev \
	libcairo2-dev \
	libcolord-dev \
	libdbus-1-dev \
	libdrm-dev \
	libegl1-mesa-dev \
	libelf-dev \
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
	libjack-jackd2-dev \
	libjpeg-dev \
	libjpeg-dev \
	liblcms2-dev \
	libmtdev-dev \
	libpam0g-dev \
	libpango1.0-dev \
	libpciaccess-dev \
	libpixman-1-dev \
	libpng-dev \
	libpulse-dev \
	libsbc-dev \
	libsystemd-dev \
	libtool \
	libudev-dev \
	libva-dev \
	libvpx-dev \
	libvulkan-dev \
	libwebp-dev \
	libx11-dev \
	libx11-xcb-dev \
	libxcb1-dev \
	libxcb-composite0-dev \
	libxcb-dri2-0-dev \
	libxcb-dri3-dev \
	libxcb-glx0-dev \
	libxcb-present-dev \
	libxcb-randr0-dev \
	libxcb-shm0-dev \
	libxcb-sync-dev \
	libxcb-xfixes0-dev \
	libxcb-xkb-dev \
	libxcursor-dev \
	libxcb-cursor-dev \
	libxdamage-dev \
	libxext-dev \
	libxfixes-dev \
	libxkbcommon-dev \
	libxml2-dev \
	libxxf86vm-dev \
	lld-11 \
	llvm-11 \
	llvm-11-dev \
	mesa-common-dev \
	ninja-build \
	pkg-config \
	python3-pip \
	python3-pygments \
	python3-setuptools \
	qemu-system \
	sysvinit-core \
	x11proto-dev \
	xwayland \
	$MESA_DEV_PKGS \
	$MESA_RUNTIME_PKGS \
	$LINUX_DEV_PKGS \


# Actually build our dependencies ...
./.gitlab-ci/build-deps.sh


# And remove packages which are only required for our build dependencies,
# which we don't need bloating the image whilst we build and run Weston.
apt-get -y --autoremove purge $LINUX_DEV_PKGS $MESA_DEV_PKGS
