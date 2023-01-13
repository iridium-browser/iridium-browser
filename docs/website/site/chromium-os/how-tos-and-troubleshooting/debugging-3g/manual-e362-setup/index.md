---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/debugging-3g
  - Debugging a cellular modem
page_name: manual-e362-setup
title: 'LTE: Manual E362 setup'
---

## LTE Support on Link

**E362 modems are supported on the Link platform because the make.conf file
specifies USE=modemmanager-next. There should be nothing special you need to do,
other than to make sure that you are running shill and not flimflam.**

---

## LTE Support on Alex

**For developers using older hardware, e.g. Alex, the following instructions are
still relevant.**

**How to set up to manually test and use a Novatel E362 LTE modem (updated 2012-2-14)**
**(This should all be handled by Shill in the near future)**
**You will need:**
**- ChromeOS dev system**
**- ChromeOS device with an E362 + appropriate SIM**
**On your dev server, in the chroot, run:**
**emerge-${BOARD} --unmerge net-misc/modemmanager modemmanager-next-interfaces
virtual/modemmanager**

start-devserver
On the device, as root, run:
stop modemmanager
mount -o remount,rw /
emerge --unmerge net-misc/modemmanager modemmanager-next-interfaces
virtual/modemmanager
Set up the network to dev server, USB ethernet or whatever, in your usual
manner, and run:
USE=modemmanager_next gmerge --accept_stable virtual/modemmanager
start modemmanager

mmcli -L
mmcli -m 0
Confirm that the modem is detected - this command should produce about 20 lines
of status about the device.
To connect:
ifconfig eth0 down
mmcli -m 0 --simple-connect=""
ifconfig eth0 up
This last command triggers flimflam to run DHCP and configure the interface.
-&gt; Make sure you unplug USB Ethernet devices and disable wifi before running
speedtest or similar!
To disconnect:
mmcli -m 0 --simple-disconnect
