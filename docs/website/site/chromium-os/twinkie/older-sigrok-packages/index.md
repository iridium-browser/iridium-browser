---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/twinkie
  - USB-PD Sniffer
page_name: older-sigrok-packages
title: Older Sigrok packages and firmwares
---

#### Previous versions of the binary packages for x86_64

#### - for Ubuntu Xenial (16.04 LTS)

#### ```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-xenial/libsigrok4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-xenial/libsigrokcxx4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-xenial/libsigrokdecode4_0.5.0-4twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-xenial/sigrok-cli_0.7.0-2twinkie1_amd64.deb \
      https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-xenial/pulseview_0.4.0-2twinkie1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

#### - for Debian Buster (testing)

#### ```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-buster/libsigrok4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-buster/libsigrokcxx4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-buster/libsigrokdecode4_0.5.0-4twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-buster/sigrok-cli_0.7.0-2twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-buster/pulseview_0.4.0-2twinkie1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

#### - for Debian Stretch (stable)

#### ```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-stretch/libsigrok4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-stretch/libsigrokcxx4_0.5.0-3twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-stretch/libsigrokdecode4_0.5.0-4twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-stretch/sigrok-cli_0.7.0-2twinkie1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/debian-stretch/pulseview_0.4.0-2twinkie1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

#### - for Ubuntu Trusty (14.04 LTS), you can unpack the following
[binaries](https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-trusty/sigrok-twinkie-0.5.0.tar.gz)
at the desired location on your computer and install the dependencies, e.g:

#### ```none
wget -qO- https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/ubuntu-trusty/sigrok-twinkie-0.5.0.tar.gz | tar xvz
sudo apt-get install libqt5core5a libqt5widgets5 libqt5gui5 libboost-system1.55.0 libboost-filesystem1.55.0 libboost-serialization1.55.0 libglibmm-2.4 libglib2.0
```

#### then execute them directly, e.g
`/path/to/your/sigrok-twinkie-0.5.0/bin/pulseview `

#### `or /path/to/your/sigrok-twinkie-0.5.0/bin/sigrok-cli -d chromium-twinkie
--continuous -o [`test.sr`](http://test.sr/)
#### or add them to your path, e.g `export
PATH="$PATH:/path/to/your/sigrok-twinkie-0.5.0/bin"`

#### Previous versions of the binary packages for Ubuntu Trusty, mainly for the record

the x86_64 version of the packages:

[libsigrok1_20141111-1_amd64.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20141111/libsigrok1_20141111-1_amd64.deb)

[libsigrokdecode1_20141111-1_amd64.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20141111/libsigrokdecode1_20141111-1_amd64.deb)

[sigrok-cli_20141111-1_amd64.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20141111/sigrok-cli_20141111-1_amd64.deb)

[pulseview_20141111-1_amd64.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20141111/pulseview_20141111-1_amd64.deb)

the ARM version of the packages :

[libsigrok1_0.3.0-7df7cdb-1_armhf.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20150303/libsigrok1_0.3.0-7df7cdb-1_armhf.deb)

[libsigrokdecode1_0.3.0-8a6e61d-1_armhf.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20150303/libsigrokdecode1_0.3.0-8a6e61d-1_armhf.deb)

[sigrok-cli_0.5.0-c9edfa2-1_armhf.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20150303/sigrok-cli_0.5.0-c9edfa2-1_armhf.deb)

[pulseview_20150303-321ce42-1_armhf.deb](http://storage.googleapis.com/chromeos-vpa/sigrok-20150303/pulseview_20150303-321ce42-1_armhf.deb)

Once you have downloaded the .deb packages, you can use the following
command-line to install them easily:

```none
sudo dpkg -i libsigrok1_20141111-1_amd64.deb libsigrokdecode1_20141111-1_amd64.deb sigrok-cli_20141111-1_amd64.deb pulseview_20141111-1_amd64.deb
sudo apt-get install -f
```

#### Previous released versions of the firmware binary

the*` twinkie_v1.1.2705-bc6d966.combined.bin `*firmware binary is attached at
the bottom of this page.
