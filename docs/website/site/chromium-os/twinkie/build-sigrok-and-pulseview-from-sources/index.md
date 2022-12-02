---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/twinkie
  - USB-PD Sniffer
page_name: build-sigrok-and-pulseview-from-sources
title: Build Sigrok and Pulseview from sources
---

If you want to build Sigrok and Pulseview from sources with the Twinkie support
in libsigrok,

the easiest path is to use the last released versions:

[libsigrok-0.5.0](http://sigrok.org/gitweb/?p=libsigrok.git;a=shortlog;h=refs/heads/libsigrok-0.5.x)
(with [twinkie-0.5.0
patch](https://github.com/vpalatin/libsigrok/commit/6228032ec0c86b776b80ed91bf810952fdd561a7))
[libsigrokdecode-0.5.0](http://sigrok.org/gitweb/?p=libsigrokdecode.git;a=shortlog;h=refs/heads/libsigrokdecode-0.5.x)
[sigrok-cli-0.7.0](http://sigrok.org/gitweb/?p=sigrok-cli.git;a=shortlog;h=refs/heads/sigrok-cli-0.7.x)
[pulseview-0.4.0](http://sigrok.org/gitweb/?p=pulseview.git;a=shortlog;h=refs/heads/pulseview-0.4.x)

Here is a recipe working on Ubuntu Trusty LTS:

```none
sudo apt-get install gcc g++ libtool automake autoconf libftdi-dev libusb-1.0-0-dev libglib2.0-dev check
sudo apt-get install libzip-dev libglibmm-2.4-dev doxygen python3.4-dev python-gobject-dev swig3.0
sudo apt-get install qtbase5-dev qtbase5-dev-tools libqt5svg5-dev cmake
sudo apt-get install libboost1.55-dev libboost-filesystem1.55-dev libboost-system1.55-dev libboost-test1.55-dev libboost-serialization1.55-dev
git clone https://github.com/vpalatin/libsigrok.git -b twinkie-0.5.0
git clone https://github.com/vpalatin/libsigrokdecode.git -b twinkie-0.5.0
git clone https://github.com/vpalatin/sigrok-cli.git -b twinkie-0.7.0
git clone https://github.com/vpalatin/pulseview.git -b twinkie-0.4.0
cd libsigrok
./autogen.sh
./configure --prefix=/usr
make install
cd ../libsigrokdecode/
./autogen.sh
./configure --prefix=/usr
make install
cd ../sigrok-cli/
./autogen.sh
./configure --prefix=/usr
make install
cd ../pulseview/
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr .
make install
```
