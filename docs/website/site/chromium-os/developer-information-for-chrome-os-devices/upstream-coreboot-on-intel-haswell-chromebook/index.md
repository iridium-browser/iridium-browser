---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: upstream-coreboot-on-intel-haswell-chromebook
title: Upstream coreboot on Intel Haswell Chromebook
---

### Disable write protection by following the [Chromium OS wiki](/chromium-os/developer-information-for-chrome-os-devices)

1.  Open device
2.  Remove write protect screw
3.  Boot device in developer mode
4.  Disable flash write protection

```none
flashrom --wp-disable
```

### Check out coreboot

```none
git clone http://review.coreboot.org/p/coreboot
cd coreboot
```

### Extract BIOS from running system

The BIOS from the running system will contain some specific data in GBB and VPD
regions that are unique. It is recommended to extract and save this BIOS in case
you want to restore to previous unique device information. Without a valid HWID
the system will be unable to recover or receive updates.

```none
flashrom -r device_bios.bin
```

### Extract BIOS from Firmware Update utility on the device

The BIOS extracted from a running system does not contain the Management Engine
firmware as that region is not readable from the host. The firmware update
utility contains a raw BIOS update image which will contain the Management
Engine binary.

Copy /usr/sbin/chromeos-firwmwareupdate from device, execute, and copy bios.bin
to coreboot root.

```none
chromeos-firwmareupdate --sb_extract /tmp
cp /tmp/bios.bin .
```

### Extract SPI Descriptor and Management Engine binary

```none
ifdtool -x bios.bin
```

### Extract MRC binary

```none
cbfstool bios.bin extract -n mrc.bin -f mrc.bin
```

### Extract VBIOS binary

### Save as PCI Device ID 0x0a06 to make SeaBIOS happy

```none
cbfstool bios.bin extract -n pci8086,0406.rom -f pci8086,0a06.rom
```

### Configure Coreboot

```none
make menuconfig
```

### Suggested Settings

```none
> Mainboard -> Mainboard Vendor -> Google
> Mainboard -> Mainboard Model -> Falco
> Chipset -> Add a System Agent binary -> mrc.bin
> VGA BIOS -> Add a VGA BIOS image -> pci8086,0a06.rom
> VGA BIOS -> VGA device PCI IDs -> 8086,0a06
> Console -> Serial port console output -> DISABLE
> Payload -> Add a payload -> SeaBIOS
> Payload -> SeaBIOS version -> master
> ChromeOS -> Build for ChromeOS -> DISABLE
```

### Build Coreboot and SeaBIOS

```none
make
```

### Configure SeaBIOS to use ESC key instead of F12

```none
echo -ne "\x01" > boot-menu-key
echo -e "\nPress ESC for boot menu.\n" > boot-menu-message
cbfstool build/coreboot.rom add -f boot-menu-key -n etc/boot-menu-key -t raw
cbfstool build/coreboot.rom add -f boot-menu-message -n etc/boot-menu-message -t raw
```

If setting a boot splash screen:

```none
cbfstool build/coreboot.rom add -f bootsplash.jpg -n bootsplash.jpg -t raw
```

### Add SPI Descriptor

```none
dd if=flashregion_0_flashdescriptor.bin of=build/coreboot.rom conv=notrunc
```

### Add Management Engine

```none
ifdtool -i me:flashregion_2_intel_me.bin build/coreboot.rom
mv build/coreboot.rom.new build/coreboot.rom
```
