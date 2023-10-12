---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: nfs-quickstart
title: NFS-quickstart
---

**Work In Progress.. (None of this is officially supported so CLs appreciated)**

## Build a suitable kernel and rootfs

**Option 1:** With `cros build-packages`

```bash
./set_shared_user_password.sh # Needed only the first time in your chroot so you can login to the rootfs
USE=nfs cros build-packages --board=${BOARD} --no-withfactory # no-withfactory is important. If you miss it you will boot into factory installer mode
```

With this option you can do your regular cros_workon + emerge commands (and any
optimizations like cros_workon_make etc). But you have to run the
mod_image_for_test scripts on the /build/&lt;board&gt; dir or hand edit they
etc/shadow files etc to login

**Option 2:** **With ./quickly**

(DONT USE YET - if you want incremental compiles etc. This is useful to create a
rootfs different from /build/&lt;board&gt; if you want to mod for test etc)

Quickly is a simple script (integrated ./build_packages/image) to enable
creating a rootfs ready for NFS boot. You can cherry-pick
<https://gerrit.chromium.org/gerrit/21579> into src/scripts . This is not a
supported script. It contains pieces of other scripts to speed up building a
test image for developers. If you are not comfortable with editing the script,
use option 1 above.

The advantage of using this script is

1) It creates a new rootfs dir that you can hackon/delete unlike option 1 which
reuses /build/&lt;board&gt; which is polluted with factory install hacks etc

2) It runs mod_image_for_test for you so you should be running closer to a test
image

3) If you have to do any hacks to the rootfs generated feel free to update the
./quickly script and update the CL

```none
./quickly --board=daisy --help         
./quickly --board=daisy        #this runs build_packages and parts of mod_image_for_test
```

The output is a ChromiumOS test image in
**../build/images/&lt;board&gt;/quickly/**. Use this as your rootfs in the NFS
server setup if you go down this path. You will have to run your regular emerge
commands and then run ./quickly to regenerate your rootfs again.

TODO(yournamehere):./quickly will need to incrementally update the rootfs and
emerge only packages that have been updated.

## Setting up an NFS server

**Step 1**: install the NFS server package (these instructions for Ubuntu 10.04
Lucid). This enables the NFS server built into your kernel. When it starts you
may notice that a number of new modules have been loaded into your kernel (nfsd,
exportfs, lockd, etc.)

```none
sudo apt-get install nfs-kernel-server
```

**Step 2**:The server needs to know which directories you want to 'export' for
clients. This is specified in the `/etc/exports` file. Edit this (`sudo vi
/etc/exports`) to look something like this (changing IP subnets as appropriate):

```none
/home/$USER/cros_i1/chroot 172.22.0.0/16(rw,fsid=0,nohide,no_subtree_check,async)
/home/$USER/cros_i1/chroot/build/daisy 172.22.0.0/16(rw,nohide,no_subtree_check,async,no_root_squash)
```

**Step 3**:Restart the NFS kernel server:

```none
$ sudo /etc/init.d/nfs-kernel-server restart
 * Stopping NFS kernel daemon                                            [ OK ] 
 * Unexporting directories for NFS kernel daemon...                      [ OK ] 
 * Exporting directories for NFS kernel daemon...                        [ OK ] 
 * Starting NFS kernel daemon                                            [ OK ]
```

**Step 4**:If it is working you will see something like:

```none
$ rpcinfo -p localhost |grep nfs
    100003    2   udp   2049  nfs
    100003    3   udp   2049  nfs
    100003    4   udp   2049  nfs
```

**Step 5**:Test your nfs server setup. This example just mounts the nfsroot on
the same machine, but it is useful to do this from another machine is possible
(if you can, use your server host name instead of localhost).

```none
$ sudo mkdir -p /tmp/nfs
$ sudo mount -t nfs localhost:/nfsroot /tmp/nfs4
$ ls /tmp/nfs
bin  boot  build  dev  etc  home  lib  lost+found  media  mnt  opt  postinst  proc  root  sbin  share  sys  tmp  usr  var
$ sudo umount /tmp/nfs
```

## U-Boot

Step 0: Make sure you have <https://gerrit.chromium.org/gerrit/#change,21213> in
your uboot. It enables netboot/nfs.

First, make sure that you have flashed the developer U-Boot onto your board/SD
card (aka legacy_image.bin). For more information see the [U-Boot
page](/developers/u-boot) (outdated and refers only to seaboard). Email
chromium-os-dev@ if you dont have uboot on your device. When you start the
developer U-Boot for the first time it will likely complain that the environment
is not present, and give you a default environment. Type `printenv` to see what
it defines.
TODO(anush): Save it in a nice to run command. Keep in mind running dhcp will
overwrite any serverip configuration with what is provided so we need to set
serverip after getting a dhcp IP address.

Make sure you connect to the onboard ethernet jack on Daisy NOT with a USB
Dongle

```none
setenv ethaddr 00:1A:11:XX:XX:XX                         # recommend using the hex version of your employee id to avoid collisions TODO(XXX):resolve with real MACs
dhcp                                                     # Get yourself a DHCP address. 
set nfsserverip 172.22.XX.XX                             # dhcp from the step above will overwrite your serverip
set serverip 172.22.XX.XX
set rootpath /home/$USER/cros_i1/chroot/build/daisy      # point to quickly rootfs if you use that
setenv bootdev_bootargs root=/dev/nfs rw nfsroot=${nfsserverip}:${rootpath} ip=dhcp noinitrd daisy nfsrootdebug; run regen_all
nfs /home/$USER/cros_i1/chroot/build/daisy/boot/vmlinux.uimg
bootm
```

## NFS Troubleshooting/Debugging

\* If you delete and recreate your NFS root directory make sure you restart the
NFS server

\* If you dont have network connectivity, try restarting shill from the console
(I will send out a CL to fix this)

TODO(yournamehere): Add notes on using debugging chrome with an NFS root
