---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: network-based-development
title: Network-based Development
---

## FIXME: Most of the instructions below seem to apply only to ancient boards running U-Boot.

## Motivation

\[If you are familiar with/sold on network-based development you can skip this
introduction.\]
Development with a target device, where software is built on a separate host, is
quite different from host-based development. Perhaps the biggest problem is that
the host and target have different filesystems. The host filesystem is close to
you, easy to interact with, large, fast and always available. The target
filesystem is typically inaccessible from your host, has poor tools, is small,
pretty slow and only exists when the target is running.
Common problems are:

*   getting a new kernel onto your target;
*   changing kernel parameters (kernel command line);
*   updating the root filesystem on your target;
*   installing into your target a new version of a software package you
            are working on;
*   using a GUI debugger on the host to debug software on the target;
*   accessing host data files on your target;
*   writing to log files on your host from your target.

Many ad-hoc solutions are available for the above. You can use a USB stick or SD
card to transport the kernel or a new root filesystem. You can change kernel
parameters by rebuilding the kernel. If you have a network available you can use
the scp command to copy over a program to run, copy log and data files around,
etc. You can even use sshfs to run a filesystem over the network.
These problems are worse when you want to have a lab full of targets all doing
similar things. In this case each board must be reconfigured using an SD card
and manual intervention when a kernel update is required, or a log file must be
pulled off. In fact, anything which exists on the target device is something
that might be out of date, something to check, or something that will make that
target different in undesirable ways.
In short it is desirable to keep as little as possible on the target device. If
a network is available at all stages of the boot process, you can use that
instead of the filesystem. At the minimum you need to identify the target, so it
needs a name and perhaps a serial number. Everything else can be loaded from an
attached network.
Common objections to using a network, compared to an SD card say, are:

*   "But it is slower than having local storage in the target." Network
            speed is typically 10MB/s and can be faster. You should take account
            of the time spent copying onto the SD card as well as the time the
            target takes copying it off.
*   "But it makes my target boot process different from the way it will
            actually boot in the field." True, although you can easily test it,
            and for most testing purposes (with the notable exception of
            benchmarking) the boot device seldom affects operation.
*   "But I never change my kernel / root disk and never need to change
            files anyway." You are probably not doing development. Why are you
            reading this page?
*   "But I can never be sure what the target has actually picked up."
            You can check the kernel header, and touch a file visible to the
            target which you can check.
*   "But my target doesn't have a real time clock." Most ARM SOCs have a
            serviceable one built in, but in any case you don't really need one.
            If you need to, just set the time in a startup script using an ntp
            utility or similar.
*   "But I don't have a network." You should have thought of that before
            you developed your device. Ethernet chips and connectors are cheap
            enough that they should be on your board for debug reasons alone.
            There is a valuable difference between development and production
            hardware. Failing that, perhaps a USB Ethernet adapter will help?
*   "But it is too difficult to set up; I don't understand NFS; the
            network around here is unreliable; I am allergic to NFS servers;
            tftp is insecure; I don't have a spare network port; it feels
            unnatural; I have an SD card for this purpose; my parents didn't use
            a network for embedded development when I was growing up; etc." You
            will need to work through these issues yourself.

## Chromium OS Approach

(ARM only, for now).
Our approach to network-based development is:

*   U-Boot is flashed into the board as a starting point
*   U-Boot environment variables specify boot method, including network
            servers
*   DHCP is used to obtain an IP address
*   TFTP is used to read the kernel
*   NFS Root is used to provide the root disk

The remainder of this page shows how to set this up.

## U-Boot on the board

Chromium OS has several variants of U-Boot for each board, one of which is **the
developer variant**. The developer variant has a number of additional features
useful to devs: network support, an expanded command set and a saved
environment.
First, make sure that you have flashed the developer U-Boot onto your board. For
more information see the [U-Boot page](/developers/u-boot).

### Supported USB Ethernet Adapters / USB Ethernet Dongles

Only the ASIX AX88772 chip is supported at present. The following supported
adapters are shown as {vendor, device}. Type `lsusb`, on a machine running
Linux, with the adapter attached to see the vendor and device numbers for your
adapter.
` { 0x05ac, 0x1402 }, /* Apple USB Ethernet Adapter */`
` { 0x07d1, 0x3c05 }, /* D-Link DUB-E100 H/W Ver B1 */`
` { 0x0b95, 0x772a }, /* Cables-to-Go USB Ethernet Adapter */ `TODO(dianders):
This didn't work for me.
` { 0x0b95, 0x7720 }, /* Trendnet TU2-ET100 V3.0R */`
` { 0x0b95, 0x1720 }, /* SMC */`
` { 0x0db0, 0xa877 }, /* MSI - ASIX 88772a */`
` { 0x13b1, 0x0018 }, /* Linksys 200M v2.1 */`
` { 0x1557, 0x7720 }, /* 0Q0 cable ethernet */`
` { 0x2001, 0x3c05 }, /* DLink DUB-E100 H/W Ver B1 Alternate */`

## U-Boot Environment Variables

When you start the developer U-Boot for the first time it will likely complain
that the environment is not present, and give you a default environment. Type
`printenv` to see what it defines.
To set your board up for full network booting you need to set the following.
Replace the IP addresses in the following with the address of your server. In
fact, typical U-Boot flow is to use `serverip` for all the server addresses, and
have this obtained automatically from the DHCP server. However in many corporate
network environments this is difficult.
Note: It is a great idea if your DHCP server can give you the same IP address
each time. It does this by looking at the MAC address on your board / Ethernet
adapter. Note that you should replace `**yourUsernameHere**` with the username
you'll be using on your host machine and **yourSerialNumberHere** with anything
you want. You should also change the IP address to be the IP addresses of your
servers.

<pre><code>setenv serverip 172.22.73.60
setenv tftpserverip 172.22.73.60
setenv nfsserverip 172.22.73.60
setenv board tegra2_seaboard
setenv serial# <b>yourSerialNumberHere</b>
setenv user <b>yourUsernameHere</b>
setenv tftppath /uImage-${user}-${board}-${serial{{ '#}' }}
saveenv
</code></pre>

Other variables are set automatically by the boot scripts:

```none
rootpath=/export/nfsroot-${user}-${board}-${serial{{ '#}' }}
```

There are two paths that your server must provide. The first is the path to your
kernel on your tftp server, and the second is the path to your root disk on your
NFS server. By adding the username, board and serial number it makes it possible
for a team to share a single TFTP/NFS server.
By default the IP address is obtained using DHCP. If you don't have a DHCP
server you can set the environment variables directly. You will need to create
your own boot flow which skips bootp.

```none
gatewayip=172.22.73.1
netmask=255.255.255.0
ipaddr=172.22.73.81
```

### U-Boot Boot Flow

U-Boot starts by running an environment variable called `bootcmd`. This starts
up USB then tries various ways of obtaining a kernel:

*   `dhcp_boot` - uses DHCP to get an IP address, TFTPs the kernel from
            a fixed server, then boots with NFS root from a fixed server
*   `keynfs_boot` - reads the kernel from file '/uImage' on an attached
            USB stick (first partition in ext2 format), then boots with NFS root
*   `usb_boot` - reads a developer image in standard Chromium OS format
            from a USB stick (not network boot)
*   `mmc_boot` - reads a developer image in standard Chromium OS format
            from eMMC (not network boot)

The first method to succeed is used for the boot. This means that if you don't
have a network attached, and your USB stick does not have uImage on it, then it
will try to read a Chromium OS format image from the stick. If no stick is
inserted, it will try to boot from internal eMMC.
To adjust the boot flow you should create your own boot command. For example you
might create a method that loads a kernel from partition 2 of the USB stick, so
replacing `nfskey_boot`,. Then change the `bootcmd` so it only runs your flow
(NOTE: you may need to paste it a little bit at a time):

```none
setenv mynfs_setup 'setenv rootpath /export/nfsroot-${user}-${board}-${serial{{ '#}' }}; run regen_net_bootargs'
setenv mynfs_boot 'run keynfs_setup; ext2load usb 0:2 ${loadaddr} uImage; bootm ${loadaddr}'
setenv bootcmd run mynfs_boot
saveenv
```

## Setting up a DHCP server

The DHCP server provides IP addresses to targets on your network. If you have a
DHCP server already, skip to the end of this section to test it. Otherwise you
will need to set one up. This is a very brief guide.

```none
sudo aptitude install dhcp3-server
```

Edit /etc/dhcp3/dhcpd.conf and add details about your subnet, including the
range of IP addresses you want to give out and any fixed IP addresses you want
to allocate for your targets:

```none
subnet 192.168.4.0 netmask 255.255.255.0 {
  range 192.168.4.20 192.168.4.50;
  option routers 192.168.4.1;
}
host seaboard {
  hardware ethernet 00:23:7d:09:80:0e;
  fixed-address seboard0;
}
```

(you may want to put seaboard0 in your `/etc/hosts` file in this example, or you
can use a numeric address)
Then start up the server:

```none
/etc/init.d/dhcp3-server restart
```

If there are no errors (`tail /var/log/syslog`) you should be in business.

#### Testing your DHCP server

At this stage you should be able to boot your target and see it get a valid IP
address:

```none
CrOS> usb start
(Re)start USB...
USB:   Tegra ehci init hccr c5008100 and hcor c5008140 hc_length 64
Register 10011 NbrPorts 1
USB EHCI 1.00
scanning bus for devices... 5 USB Device(s) found
       scanning bus for storage devices... 1 Storage Device(s) found
       scanning bus for ethernet devices... 1 Ethernet Device(s) found
CrOS> bootp
Waiting for Ethernet connection... done.
BOOTP broadcast 1
DHCP client bound to address 172.22.73.81
CrOS> 
```

### Setting up a TFTP server

The TFTP server will send a kernel to U-Boot when it asks. If you have one
already skip to the end of this section to test it.

```none
sudo aptitude install tftpd-hpa
```

Then edit `/etc/default/tftpd-hpa` like this replacing **yourUsernameHere** with
your own username (`echo $USER`):

<pre><code>TFTP_USERNAME="<b>yourUsernameHere</b>"
TFTP_DIRECTORY="/tftpboot"
TFTP_ADDRESS="0.0.0.0:69"
TFTP_OPTIONS="-v"
</code></pre>

Notes:

*   We remove --secure from default config, since we want to access a
            symlink that doesn't exist in the `tftpboot` directory.
*   We move from `/var/lib/tftpboot` to `/tftpboot`, since we need to
            specify full path when `--secure` isn't needed and `/tftpboot` is
            shorter.
*   We don't use the `tftp` username, since it can't access the kernel
            we build.

Suggestions for good workflows that fix some of the above would be appreciated.

Now prepare it. We want a symlink from the /tftpboot directory to you kernel, so
the target can easily read it. Replace the filename with your user, board and
serial. NOTE: this assumes that you've got your chromiumos source code in
`/home/$USER/chromiumos`:

<pre><code>cd /tftpboot
sudo ln -s /home/$USER/chromiumos/chroot/build/tegra2_seaboard/boot/vmlinux.uimg uImage-$USER-tegra2_seaboard-<b>yourSerialNumberHere</b>
sudo restart tftpd-hpa
</code></pre>

#### Testing your TFTP server

Ensure that you have an IP address (as shown in the DHCP section above). Then
this should read in the kernel (assuming you've built it):

```none
tftpboot ${loadaddr} ${tftpserverip}:/tftpboot/uImage-${user}-${board}-${serial{{ '#}' }}
```

You'll see this output if things are working well:

```none
Waiting for Ethernet connection... done.
Using asx0 device
TFTP from server 172.22.73.60; our IP address is 172.22.73.81
Filename '/tftpboot/uImage-yourUsernameHere-seaboard-yourSerialNumberHere'.
Load address: 0x40c000
Loading: #################################################################
     #################################################################
     #################################################################
     ###############################################
done
Bytes transferred = 3545596 (3619fc hex)
```

If you have got this far, congratulations! You are about half way there: the
target is reading a kernel directly from your machine and is ready to boot it.
Now we need to get the root disk organized.

## NFS Root

NFS root is a simple way to keep your target's root disk be kept on the network.

Advantages:

*   You can make a change anywhere in the filesystem from your host, and
            the target see it almost immediately and without any special manual
            transfer
*   You can make use of the effectively infinite disc space of most
            hosts (great for debug builds)
*   Build / debug turnaround time is quick since there is no need to
            transfer the image via USB. (But see also the dev_server)
*   You can set things up with symlinks so that the host and target can
            use the same filepaths

Disadvantages:

*   It takes a bit of setup
*   Slower than a local disc unless you have a fast Gigabit Ethernet
            (non-USB)

Linux requires a root filesystem which it mounts immediately after booting. If
you have a suitable network device attached to the client you can use an NFS
server to provide this root filesystem. This means that your client can have
full access to your server filesystem, thus removing the need to manually move
files from the host to the client for execution, and back again for editing /
viewing.
You can find information about setting up NFS root
[here](https://wiki.archlinux.org/index.php/NFSv4). These instructions replicate
much of that, and generally follow the same pattern but are more specific to
Chromium OS. Note that you can use NFSv3 instead but here we will use NFSv4 as
it has additional features.
The steps you need to take are:

*   Set up an NFS server on your host, or use one that you already have
*   Build a kernel with suitable filesystem and driver options
*   Set up your boot loader to tell the kernel to use an NFS root

### Setting up an NFS server

First, install the NFS server package (these instructions for Ubuntu 10.04
Lucid). This enables the NFS server built into your kernel. When it starts you
may notice that a number of new modules have been loaded into your kernel (nfsd,
exportfs, lockd, etc.)

```none
sudo aptitude install nfs-kernel-server
```

The server needs to know which directories you want to 'export' for clients.
This is specified in the `/etc/exports` file. Edit this (`sudo nano
/etc/exports` or similar) to look something like this (changing IP subnets as
appropriate):

<pre><code>/export            172.16.0.0/16(rw,fsid=0,no_subtree_check,async)
/export/nfsroot-<b>yourUsernameHere</b>-tegra2_seaboard-<b>yourSerialNumberHere</b>    172.16.0.0/16(rw,nohide,no_subtree_check,async,no_root_squash)
</code></pre>

The first entry sets the base of the NFS exports. Every directory which is
exported must be accessible from within `/export`. The second entry is the
nfsroot directory which will contain your root filesystem. This is the directory
that the client will see when it mounts the NFS root. The IP address should be
changed to match your local setup - it sets the range of IP addresses which are
allowed to access this mount on the server. For example it might be
`192.168.1.0/24`.
A number of options are provided, briefly:

*   **rw** - the target will have both read and write access. You can
            also use ro for readonly but the system will not boot with a
            read-only root filesystem (without a bit of work!)
*   **fsid=0** - tells NFS that this is the root of all exported
            filesystems
*   **no_subtree_check** disables checking for accesses outside the
            exported portion of a filesystem. This speeds up and simplifies
            things for the client and server.
*   **async** - requests are acknowledged before data is actually
            written. For example if the client writes to a file, the server will
            respond that the write has completed, and then continue in the
            background to actually do the write, perhaps to a disc drive. This
            improves performance.
*   **no_root_squash** - the target can access files as root, with full
            unrestricted permissions. This is important for the root filesystem
            because the kernel would otherwise not have access to devices in
            /dev, log files in /var/log, etc.
*   **nohide** - tells the NFS server to show the contents of a
            directory even if it is mounted from elsewhere

We now need to make the root filesystem appear in `/export/nfsroot`. Rather than
just copy it there, we will use a 'bind mount' to paste the true location onto
`/export/nfsroot`. First we need to unpack a suitable image (see the build
instructions for how to build an image).
Let's assume that you have your Chromium trunk directory as `~/cosarm` and you
are using a `tegra2_seaboard` build:

```none
# go to the directory with the latest build
cd ~/cosarm/src/build/images/tegra2_seaboard/latest
# mount it into /tmp/m
~/cosarm/src/scripts/mount_gpt_image.sh -f . -i chromiumos_test_image.bin
# copy out the contents of the image
sudo cp -a /tmp/m nfsroot
# unmount the image from /tmp/m
~/cosarm/src/scripts/mount_gpt_image.sh -u
```

This will put a full copy of the build image root disc into
`~/cosarm/src/build/images/tegra2_seaboard/latest/nfsroot`. Now we need to make
it appear in `/export/nfsroot`. Edit your `/etc/fstab` file with the full path:

<pre><code>/full/path/to/cosarm/src/build/images/tegra2_seaboard/latest/nfsroot /export/nfsroot-<b>yourUsernameHere</b>-tegra2_seaboard-<b>yourSerialNumberHere</b>    none    bind    0 0
</code></pre>

Note this must appear all on one line and you can use tabs or spaces between
fields. See `man fstab` for more information.

You will need to create an /export/nfsroot directory:

<pre><code>$ sudo mkdir -p /export/nfsroot-<b>yourUsernameHere</b>-tegra2_seaboard-<b>yourSerialNumberHere</b>
</code></pre>

This will be activated automatically when your server reboots, but since it is
already running, ask it to mount this now. After the mount you will see that the
root filesystem has appeared at `/export/nfsroot` as desired.

<pre><code>$ sudo mount /export/nfsroot-<b>yourUsernameHere</b>-tegra2_seaboard-<b>yourSerialNumberHere</b>
$ ls /export/nfsroot-<b>yourUsernameHere</b>-tegra2_seaboard-<b>yourSerialNumberHere</b>/
bin   dev  home  lost+found  mnt  postinst  root  share  tmp     usr
boot  etc  lib   media       opt  proc      sbin  sys    u-boot  var
$
</code></pre>

Check that your `/etc/idmapd.conf` is correct. You can leave the domain as is if
you like. If you change it, be careful that the same domain is used on the
client side (not relevant for NFS root though). This file sets up mapping of
user and group names between the client and server and is a key benefit of NFSv4
over NFSv3.

```none
[General]
Verbosity = 0
Pipefs-Directory = /var/lib/nfs/rpc_pipefs
Domain = local.domain.edu
[Mapping]
Nobody-User = nobody
Nobody-Group = nogroup
```

Also check your portmap settings. If this is wrong then the boot will fail with
a message like 'VFS: Unable to mount root fs via NFS, trying floppy.'. The
settings are in `/etc/defaults/portmap`. Make sure it is set to provide the
service to other machines. If your options are set to "-i 127.0.0.1" then it
will only work locally (not a useful server!). Change it to `OPTIONS=""` and
then 'restart portmap'. You can test this:

```none
$ rpcinfo -p 172.22.73.60    # use hostname or IP address of your NFS server
rpcinfo: can't contact portmapper: RPC: Remote system error - Connection refused
```

If it is working you will see something like:

```none
$ rpcinfo -p 172.22.73.60 |grep nfs
    100003    2   udp   2049  nfs
    100003    3   udp   2049  nfs
    100003    4   udp   2049  nfs
    100003    2   tcp   2049  nfs
    100003    3   tcp   2049  nfs
    100003    4   tcp   2049  nfs
```

Restart the NFS kernel server if you are superstitious:

```none
$ sudo /etc/init.d/nfs-kernel-server restart
 * Stopping NFS kernel daemon                                            [ OK ] 
 * Unexporting directories for NFS kernel daemon...                      [ OK ] 
 * Exporting directories for NFS kernel daemon...                        [ OK ] 
 * Starting NFS kernel daemon                                            [ OK ]
```

Test your nfs server setup. This example just mounts the nfsroot on the same
machine, but it is useful to do this from another machine is possible (if you
can, use your server host name instead of localhost).

```none
$ sudo mkdir -p /tmp/nfs4
$ sudo mount -t nfs4 localhost:/nfsroot /tmp/nfs4
$ ls /tmp/nfs4
bin  boot  build  dev  etc  home  lib  lost+found  media  mnt  opt  postinst  proc  root  sbin  share  sys  tmp  usr  var
$ sudo umount /tmp/nfs4
```

Finally, the standard firewall setup in Chromium OS does not permit NFS. You may
wish to simply disable the firewall - the
`/export/nfsroot-**yourUsernameHere**-tegra2_seaboard-**yourSerialNumberHere**/etc/init/iptables.conf`
file should be changed to do this. At the top, change the `start on` line to
`start on never`. If you don't do this you will likely boot to a login prompt,
but then you will see 'NFS server not responding' messages once the firewall
kicks in.

### Build a suitable kernel

You can build an NFS-enabled kernel for Chromium OS with something like:

```none
USE=nfs emerge-tegra2_seaboard chromeos-kernel
```

This puts a kernel in /build/&lt;board&gt;/boot/vmlinux.uimg.

TODO(dianders): Maybe this is what we want?

```none
USE=nfs FEATURES="noclean" cros_workon_make --board=${BOARD} --install kernel
```

#### Longer Explanation

There are quite a few options that you need to enable in the kernel to support
NFS root. First you need to make sure that a suitable network driver is compiled
in, and secondly you need to enable all the network filesystem options. The
following list for a USB network adapter setup gives you an idea of what is
required. Some of the important options are:

*   CONFIG_USB_USBNET - enables the USB network subsystem
*   CONFIG_NET_AX8817X - enables the particular USB driver. This is a
            Cisco / Linksys 1000Mbit USB 2.0 Ethernet dongle
*   CONFIG_NETWORK_FILESYSTEMS - enables network filesystem support
*   CONFIG_NFS_COMMON, CONFIG_NFS_FS - enable NFS client in kernel
*   CONFIG_ROOT_NFS - enable NFS root function
*   CONFIG_IP_PNP, CONFIG_IP_PNP_DHCP - required if parameter ip=dhcp is
            passed to kernel

[See here](/chromium-os/how-tos-and-troubleshooting/kernel-configuration) for
more information about kernel configuration.

Note: some of the options below are required for NFS root, some for NFS mounting
and some for NFS serving.

```none
chromeos/config/armel/config.flavour.chromeos-tegra2:
CONFIG_USB_NET_AX8817X=y
chromeos/config/config.common.chromeos:
+CONFIG_DNOTIFY=y
+CONFIG_DNS_RESOLVER=y
+CONFIG_LOCKD=y
+CONFIG_LOCKD_V4=y
+CONFIG_NETWORK_FILESYSTEMS=y
+CONFIG_NFSD=m
+CONFIG_NFSD_V3=y
+CONFIG_NFSD_V4=y
+CONFIG_NFS_COMMON=y
+CONFIG_NFS_FS=y
+CONFIG_NFS_USE_KERNEL_DNS=y
+CONFIG_NFS_V3=y
+CONFIG_NFS_V4=y
+CONFIG_ROOT_NFS=y
+CONFIG_RPCSEC_GSS_KRB5=y
+CONFIG_SUNRPC=y
+CONFIG_SUNRPC_GSS=y
+CONFIG_USB_USBNET=y
+CONFIG_IP_PNP=y
+CONFIG_IP_PNP_DHCP=y
```

## Putting it all together

You are now ready to try out your new kernel. Just start up your target and type
'boot'.
A partial boot trace is shown below to show the sequence of events:

```none
U-Boot 2010.09-00199-g4c814f0-dirty (Feb 14 2011 - 16:33:28)
TEGRA2 
Board:   Tegra2 Seaboard.developer
DRAM:  1 GiB
NAND:  0 MiB
SF: Detected W25Q16B with page size 256, total 2 MiB
In:    tegra-kbc
Out:   lcd
Err:   lcd
Net:   No ethernet found.
Hit any key to stop autoboot:  2
CrOS> boot
(Re)start USB...
USB:   Tegra ehci init hccr c5008100 and hcor c5008140 hc_length 64
Register 10011 NbrPorts 1
USB EHCI 1.00
scanning bus for devices... 5 USB Device(s) found
       scanning bus for storage devices... 1 Storage Device(s) found
       scanning bus for ethernet devices... 1 Ethernet Device(s) found
Waiting for Ethernet connection... done.
BOOTP broadcast 1
BOOTP broadcast 2
*** Unhandled DHCP Option in OFFER/ACK: 208
*** Unhandled DHCP Option in OFFER/ACK: 208
DHCP client bound to address 172.22.73.81
Waiting for Ethernet connection... done.
Using asx0 device
TFTP from server 172.22.73.60; our IP address is 172.22.73.81
Filename '/tftpboot/uImage-sjg-seaboard-261347'.
Load address: 0x40c000
Loading: #################################################################
     #################################################################
     #################################################################
     ###############################################
done
Bytes transferred = 3545596 (3619fc hex)
## Booting kernel from Legacy Image at 0040c000 ...
   Image Name:   Linux-2.6.37-00890-g3506cb6c-dir
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    3545532 Bytes = 3.4 MiB
   Load Address: 00008000
   Entry Point:  00008000
   Verifying Checksum ... OK
   Loading Kernel Image ... OK
OK
Starting kernel ...
Uncompressing Linux... done, booting the kernel.
[    0.000000] Initializing cgroup subsys cpuset
[    0.000000] Initializing cgroup subsys cpu
[    0.000000] Linux version 2.6.37-00890-g3506cb6c-dirty (sjg@kiwi.mtv.corp.google.com) (gcc version 4.4.3 (Gentoo Hardened 4.4.3-r4 p1.2, pie-0.4.1) ) #25 SMP PREEMPT Mon Feb 14 14:16:06 PST 2011
[    0.000000] CPU: ARMv7 Processor [411fc090] revision 0 (ARMv7), cr=10c53c7f
[    0.000000] CPU: VIPT nonaliasing data cache, VIPT aliasing instruction cache
[    0.000000] Machine: seaboard
[    0.000000] Ignoring unrecognised tag 0x54410008
[    0.000000] Memory policy: ECC disabled, Data cache writealloc
[    0.000000] PERCPU: Embedded 7 pages/cpu @c0f8a000 s7264 r8192 d13216 u32768
[    0.000000] Built 1 zonelists in Zone order, mobility grouping on.  Total pages: 227328
[    0.000000] Kernel command line: console=ttyS0,115200n8 lp0_vec=0x2000@0x1C406000 mem=384M@0M nvmem=128M@384M mem=512M@512M noinitrd dev=/dev/nfs4 rw nfsroot=172.22.73.60:/export/nfsroot-sjg-seaboard-261347 ip=dhcp
[    0.000000] PID hash table entries: 4096 (order: 2, 16384 bytes)
...
[   24.249981] usb 3-1.1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[   24.249991] usb 3-1.1: Product: AX88x72A
[   24.249998] usb 3-1.1: Manufacturer: ASIX Elec. Corp.
[   24.250005] usb 3-1.1: SerialNumber: 000001
[   25.196440] asix 3-1.1:1.0: eth0: register 'asix' at usb-tegra-ehci.2-1.1, ASIX AX88772 USB 2.0 Ethernet, 68:7f:74:9f:f6:f2
[   25.303359] usb 3-1.2: new high speed USB device using tegra-ehci and address 4
...
[   25.845564] eth0: link down
[   25.854603] ADDRCONF(NETDEV_UP): eth0: link is not ready
...
[   26.863238] Sending DHCP requests .
[   27.198754] ADDRCONF(NETDEV_CHANGE): eth0: link becomes ready
[   27.206441] eth0: link up, 100Mbps, full-duplex, lpa 0xCDE1
[   29.223214] ., OK
[   29.283223] IP-Config: Got DHCP answer from 172.24.11.52, my address is 172.22.73.81
[   29.292032] IP-Config: Complete:
[   29.295104]      device=eth0, addr=172.22.73.81, mask=255.255.252.0, gw=172.22.75.254,
[   29.302952]      host=seaboard0.mtv.corp.google.com, domain=mtv.corp.google.com, nis-domain=(none),
[   29.312000]      bootserver=172.24.11.52, rootserver=172.22.73.60, rootpath=
[   29.319579] mount nfs, dev=172.22.73.60:/export/nfsroot-sjg-seaboard-261347, flags=32768, root_data=nolock,addr=172.22.73.60VFS: Mounted root (nfs filesystem) on device 0:16.
[   29.345192] mount nfs done
[   29.348171] devtmpfs: mounted
[   29.351372] Freeing init memory: 268K
[   29.357144] Not activating Mandatory Access Control now since /sbin/tomoyo-init doesn't exist.
[   29.956410] mount used greatest stack depth: 4884 bytes left
[   30.736770] ply-image used greatest stack depth: 4668 bytes left
[   31.070665] rm used greatest stack depth: 4620 bytes left
[   31.227262] mknod used greatest stack depth: 4388 bytes left
[   31.428835] chown used greatest stack depth: 4372 bytes left
[   31.673332] chown used greatest stack depth: 4292 bytes left
[   31.755995] unknown ioctl code
[   31.759218] NvRmIoctls_NvRmFbControl: deprecated 
NVRM_DAEMON(1261): skipping Audi[   31.779495] nvrm_notifier_store: nvrm_daemon=801
oFx initialization
[   31.792064] nvrm_notifier_show: blocking
[   31.852343] mkdir used greatest stack depth: 3956 bytes left
udevd[837]: unknown key 'HOTPLUG' in /etc/udev/rules.d/99-monitor-hotplug.rules:2
[   32.327921] udevd (853): /proc/853/oom_adj is deprecated, please use /proc/853/oom_score_adj instead.
Developer Console
To return to the browser, press:
  [ Ctrl ] and [ Alt ] and [ F1 ]
To use this console, the developer mode switch must be engaged.
Doing so will destroy any saved data on the system.
In developer mode, it is possible to
- login and sudo as user 'chronos'
- require a password for sudo and login(*)
- disable power management behavior (screen dimming):
  sudo initctl stop powerd
- install your own operating system image!
* To set a password for 'chronos', run the following as root:
echo "chronos:$(openssl passwd -1)" > /mnt/stateful_partition/etc/devmode.passwd
Have fun and send patches!
localhost login: 
```

TODO: Troubleshooting?

### NFS Caveats

Some things to be aware of:

*   If your network goes down, so does your mount. If you are using NFS
            root on eth0, then typing `ifconfig eth0 down` will lock your target
            since the root filesystem goes away. Recovery is via a reboot
*   If the kernel cannot find the NFS server, or has a permissions
            problem, your kernel will not boot
