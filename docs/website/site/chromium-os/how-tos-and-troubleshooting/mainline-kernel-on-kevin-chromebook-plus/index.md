---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: mainline-kernel-on-kevin-chromebook-plus
title: Arch Linux + Mainline kernel on kevin (Chromebook Plus)
---

**Note:** Adapted/inspired by [this
page](https://archlinuxarm.org/platforms/armv7/rockchip/asus-chromebook-flip-c100p)

### Introduction

This guide will walk you through installed Arch Linux with a mainline kernel on
a USB key suitable for booting on the Chromebook Plus (kevin).

### Preparing the USB key

Plug the USB key into your **chromebook** and run the following commands in the
shell to create a new partition table and 2 partitions - one for the kernel and
another for rootfs.

<pre><code>
; umount /dev/sda*
; fdisk /dev/sda
Command (m for help): g
Command (m for help): w
Command (m for help): q
; cgpt create /dev/sda
; cgpt add -i 1 -t kernel -b 8192 -s 65536 -l Kernel -S 1 -T 5 -P 10 /dev/sda
; cgpt show /dev/sda
       <b>start</b>        size    part  contents
           0           1          PMBR
           1           1          Pri GPT header
        8192       65536       1  Label: "Kernel"
                                  Type: ChromeOS kernel
                                  UUID: A1E0BF2A-3687-A949-86F8-40507B1D0756
                                  Attr: priority=10 tries=5 successful=1
       73728   122470755       2  Label: "Root"
                                  Type: Linux data
                                  UUID: A3590EA0-A89B-434F-B6C3-902C0A87CDFB
   <b><i>122544483</i></b>          32          <b>Sec GPT table</b>
   122544515           1          Sec GPT header
</code></pre>

The numbers above will differ depending on the size of your USB key. Grab the
value under the "start" column for the "Sec GPT table" row and insert it in the
command below (&lt;start_value&gt;)

<pre><code>; cgpt add -i 2 -t data -b 73728 -s `expr <b>&lt;start_value&gt;</b> - 73728` -l Root /dev/sda
; sudo partx -a /dev/sda
; sudo mkfs.ext4 /dev/sda2
</code></pre>

Your USB key is now formatted in such a way that the chromebook can book from
it. Next step is to put something bootable on it.

### Download Arch rootfs

The following will download the latest Arch Linux image for gru-kevin and flash
the rootfs partition.

```none
; cd /tmp
; wget http://os.archlinuxarm.org/os/ArchLinuxARM-gru-latest.tar.gz
; mkdir root
; sudo mount /dev/sda2 root
; sudo tar -xf ArchLinuxARM-gru-latest.tar.gz -C root
```

In order to test the image and your partition table, flash the kernel partition
from the Arch tarball to your USB key

```none
; sudo dd if=root/boot/vmlinux.kpart of=/dev/sda1
; sync
```

Finally, unmount the root partition:

```none
; umount root
```

Unplug the USB key from your workstation and test boot it on your kevin by
pressing Ctrl+U at the developer screen. The device should boot to a login
prompt (user: root, passwd: root).

```none
; reboot
```

### Preparing the kernel

Back on your workstation, sync a linux-next kernel for maximum freshness.

~~Apply the following patches to enable display:~~

[~~https://github.com/mmind/linux-rockchip/commit/9a35625d79210a8a919fdf6c8f878520134c3a6c~~](https://github.com/mmind/linux-rockchip/commit/9a35625d79210a8a919fdf6c8f878520134c3a6c)

~~<https://github.com/mmind/linux-rockchip/commit/0b50af4d2440404d5e10303cc84e43e84f84f374>~~

**seanpaul:** ^^ These links are broken now (thanks to Brian Carnes for the
report). I don't remember what they did, but I also don't think they're needed
any longer since rockchip display support is all in mainline.

### Building the kernel

You'll need to grab the kernel.keyblock and kernel_data_key.vbprivk files in
order to package the kernel. They're stored in Arch Linux's PKGBUILDs repo on
github
[here](https://github.com/archlinuxarm/PKGBUILDs/tree/master/core/linux-gru). I
put them in &lt;kernel_root&gt;/resources/gru-kevin/.

Also stored in &lt;kernel_root&gt;/resources/gru-kevin is the
rk3399-gru-kevin.its file. Create and copy/paste the following into it:

```none
/dts-v1/;
/ {
	description = "Chrome OS kernel image with one or more FDT blobs";
	#address-cells = <1>;
	images {
		kernel@1 {
			data = /incbin/("../../.build_arm64/arch/arm64/boot/Image");
			type = "kernel_noload";
			arch = "arm64";
			os = "linux";
			compression = "none";
			load = <0>;
			entry = <0>;
		};
		fdt@1 {
			
			description = "rk3399-gru-kevin";
			data = /incbin/("../../.build_arm64/arch/arm64/boot/dts/rockchip/rk3399-gru-kevin.dtb");
			type = "flat_dt";
			arch = "arm64";
			compression = "none";
			hash@1 {
				algo = "sha1";
			};
		};
	};
	configurations {
		default = "conf@1";
		conf@1 {
			kernel = "kernel@1";
			fdt = "fdt@1";
		};
	};
};
```

Grab my build script from [github](https://github.com/atseanpaul/build_kernel),
and add a build.ini file in the kernel root with the following (changing the
bold parts to suit your environment):

<pre><code>
[target]
; From /dev/by-partuuid
kernel_part_uuid=<b>27c7f096-0f26-d544-a142-7273e6dcf01e</b>
; From /dev/by-uuid
root_uuid=<b>89d88389-c5c3-4f88-9ea6-2efd6124bc49</b>
[build]
; Choose one of these, not both. defconfig builds from an existing defconfig,
; whereas config_file points to a config file that will used verbatim
defconfig=defconfig
;config_file=&lt;path to config file&gt;
; For the kernel build command; required
kernel_arch=arm64
cross_compile=aarch64-linux-gnu-
jobs=40
; vbutil_kernel arguments, optional
vbutil_kernel=<b>/home/seanpaul/s/cros/chroot/usr/bin/vbutil_kernel</b>
keyblock=<b>resources/gru-kevin/kernel.keyblock</b>
data_key=<b>resources/gru-kevin/kernel_data_key.vbprivk</b>
cmdline=console=tty1 console=ttyS2,115200n8 earlyprintk=ttyS2,115200n8 init=/sbin/init root=PARTUUID=%U/PARTNROFF=1 rootwait rw noinitrd
vbutil_arch=aarch64
; mkimage arguments, optional
mkimage=<b>/home/seanpaul/s/cros/chroot/usr/bin/mkimage</b>
its_file=<b>resources/gru-kevin/rk3399-gru-kevin.its</b>
; post-build options
install_modules=yes
install_dtbs=yes
generate_htmldocs=no
</code></pre>

Simply run the build_kernel.py script from the kernel root directory to build
and flash your kernel. If all goes well, you should now have a USB key that will
boot a mainline kernel with Arch Linux rootfs.
