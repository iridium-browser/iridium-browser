---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: chrome-with-libcras-on-gprecise
title: Chrome with libcras on gPrecise
---

How to build and install cras and tell Chrome to use it on gooBuntu gPrecise.

**Install prerequisites.**

sudo apt-get install git libasound-dev libspeexdsp-dev libgtest-dev libtool
automake libdbus-1-dev libudev-dev g++ ladspa-sdk libsndfile-devOn
Trusty:apt-get install libsbc-devPre-Trusty:install libsbc 1.1 manually by
downloading and compiling the tarball from <http://www.bluez.org/sbc-11> (I
couldn't find a package for Precise).
**ini parser.**git clone https://github.com/ndevilla/iniparser.git cd iniparser
make sudo cp libiniparser.\* /usr/local/lib sudo cp src/dictionary.h
src/iniparser.h /usr/local/include sudo chmod 644
/usr/local/include/dictionary.h /usr/local/include/iniparser.h sudo chmod 644
/usr/local/lib/libiniparser.a sudo chmod 755
/usr/local/lib/libiniparser.so.0**gtest.**cd /usr/src/gtest sudo apt-get install
cmake sudo cmake . sudo make sudo chmod 644 \*.a sudo cp \*.a
/usr/local/lib**webrtc-audio-processing**wget
http://freedesktop.org/software/pulseaudio/webrtc-audio-processing/webrtc-audio-processing-0.3.tar.xz
tar xvf webrtc-audio-processing-0.3.tar.xz Apply
[change-header-location.patch](https://chromium-review.googlesource.com/c/499813/)
webrtc-audio-processing-0.3 ./configure make sudo make install**Build/install
cras.**git clone https://chromium.googlesource.com/chromiumos/third_party/adhd
cd adhd/cras ./git_prepare.sh ./configure --prefix=/usr make -j33 check sudo
make installmkdir /var/run/cras
chown $USER /var/run/crasIf you see some build errors on
'snd_pcm_chmap_query_t', it means that you need newer version of alsa-lib.
1.0.27 or higher is required to build cras.
**Confiugre/build Chrome.**Follow instruction on wiki for getting and building
chrome.cd chromium/srcGYP_GENERATORS="ninja" ./build/gyp_chromium
-Duse_cras=1ninja -C out/Release chrome
**Start cras and chrome.**cd adhd/crassrc/crascd
chromium/src./out/Release/chrome --use-cras
