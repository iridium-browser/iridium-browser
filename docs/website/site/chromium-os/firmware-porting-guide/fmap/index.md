---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
page_name: fmap
title: FMAP
---

Flash Map (FMAP) is a simple specification for layout of flash devices. A
reference implementation is available at [flashmap git
repo](https://chromium.googlesource.com/chromiumos/third_party/flashmap).

Below is the FMAP defined for [Pixel 2 /
Samus](/chromium-os/developer-information-for-chrome-os-devices/chromebook-pixel-2015),
retrieved by running *dump_fmap image.bin*:

fmap_signature __FMAP__

fmap_version: 1.0

fmap_base: 0x0

fmap_size: 0x00800000 (8388608)

fmap_name: FMAP

fmap_nareas: 33

&gt;&gt;&gt; This section describes the attributes of this FMAP. Note that the
Samus FW image size is 8MB.

area: 1

area_offset: 0x00000000

area_size: 0x00200000 (2097152)

area_name: SI_ALL

&gt;&gt;&gt; This region contains the SPI descriptor in a format defined by the
on-chip SPI controller and Intel Management Engine.

area: 2

area_offset: 0x00000000

area_size: 0x00001000 (4096)

area_name: SI_DESC

&gt;&gt;&gt; SPI descriptor.

area: 3

area_offset: 0x00001000

area_size: 0x001ff000 (2093056)

area_name: SI_ME

&gt;&gt;&gt; Management Engine.

area: 4

area_offset: 0x00200000

area_size: 0x00600000 (6291456)

area_name: SI_BIOS

&gt;&gt;&gt; Non descriptor / ME region.

area: 5

area_offset: 0x00200000

area_size: 0x000f0000 (983040)

area_name: RW_SECTION_A

&gt;&gt;&gt; Region A of re-writable FW, which may be reflashed during
[auto-update](/chromium-os/chromiumos-design-docs/autoupdate-details).

area: 6

area_offset: 0x00200000

area_size: 0x00010000 (65536)

area_name: VBLOCK_A

&gt;&gt;&gt; Keyblock for RW Region A.

area: 7

area_offset: 0x00210000

area_size: 0x000b0000 (720896)

area_name: FW_MAIN_A

&gt;&gt;&gt; Rewritable ramstage BIOS image A.

area: 8

area_offset: 0x002c0000

area_size: 0x00010000 (65536)

area_name: PD_MAIN_A

&gt;&gt;&gt; PD MCU (USB-PD / USB-C power controller) EC image A.

area: 9

area_offset: 0x002d0000

area_size: 0x0001ffc0 (131008)

area_name: EC_MAIN_A

&gt;&gt;&gt; Main EC ([Embedded Controller](/chromium-os/ec-development)) image
A.

area: 10

area_offset: 0x002effc0

area_size: 0x00000040 (64)

area_name: RW_FWID_A

&gt;&gt;&gt; ID string of rewritable ramstage BIOS image A. Eg.
"Google_Samus.6300.174.0".

area: 11

area_offset: 0x002f0000

area_size: 0x000f0000 (983040)

area_name: RW_SECTION_B

&gt;&gt;&gt; Region B of re-writable FW. For redundancy / recovery on failed
update.

area: 12

area_offset: 0x002f0000

area_size: 0x00010000 (65536)

area_name: VBLOCK_B

area: 13

area_offset: 0x00300000

area_size: 0x000b0000 (720896)

area_name: FW_MAIN_B

area: 14

area_offset: 0x003b0000

area_size: 0x00010000 (65536)

area_name: PD_MAIN_B

area: 15

area_offset: 0x003c0000

area_size: 0x0001ffc0 (131008)

area_name: EC_MAIN_B

area: 16

area_offset: 0x003dffc0

area_size: 0x00000040 (64)

area_name: RW_FWID_B

&gt;&gt;&gt; RW Region B.

area: 17

area_offset: 0x003e0000

area_size: 0x00010000 (65536)

area_name: RW_MRC_CACHE

&gt;&gt;&gt; Region that stores board DRAM timing information, written during
initial memory training.

area: 18

area_offset: 0x003f0000

area_size: 0x00004000 (16384)

area_name: RW_ELOG

&gt;&gt;&gt; Event log. Normally exported and decoded on a Chromebook to
/var/log/eventlog.

area: 19

area_offset: 0x003f4000

area_size: 0x00004000 (16384)

area_name: RW_SHARED

area: 20

area_offset: 0x003f4000

area_size: 0x00002000 (8192)

area_name: SHARED_DATA

area: 21

area_offset: 0x003f6000

area_size: 0x00002000 (8192)

area_name: VBLOCK_DEV

&gt;&gt;&gt; Shared FW data related to dev-mode FW verification.

area: 22

area_offset: 0x003f8000

area_size: 0x00002000 (8192)

area_name: RW_VPD

&gt;&gt;&gt; Writable vital product data, eg. first use date.

area: 23

area_offset: 0x003fa000

area_size: 0x00006000 (24576)

area_name: RW_UNUSED

&gt;&gt;&gt; Extra / unused space in RW region.

area: 24

area_offset: 0x00400000

area_size: 0x00200000 (2097152)

area_name: RW_LEGACY

&gt;&gt;&gt; Alternate OtherOS payload, normally for booting SeaBIOS.

area: 25

area_offset: 0x00600000

area_size: 0x00200000 (2097152)

area_name: WP_RO

&gt;&gt;&gt; RO region of FW - written once at the factory and (normally) never
again.

area: 26

area_offset: 0x00600000

area_size: 0x00004000 (16384)

area_name: RO_VPD

&gt;&gt;&gt; RO vital product data, eg. sensor calibration data, keyboard /
region information.

area: 27

area_offset: 0x00604000

area_size: 0x0000c000 (49152)

area_name: RO_UNUSED

&gt;&gt; Extra . unused space.

area: 28

area_offset: 0x00610000

area_size: 0x001f0000 (2031616)

area_name: RO_SECTION

&gt;&gt;&gt; RO FW region.

area: 29

area_offset: 0x00610000

area_size: 0x00000800 (2048)

area_name: FMAP

&gt;&gt; Data structure that describes what you're looking at right now :)

area: 30

area_offset: 0x00610800

area_size: 0x00000040 (64)

area_name: RO_FRID

&gt;&gt;&gt; ID string of RO BIOS image. Eg. "Google_Samus.6300.174.0".

area: 31

area_offset: 0x00610840

area_size: 0x000007c0 (1984)

area_name: RO_FRID_PAD

&gt;&gt;&gt; For alignment of RO_FRID / GBB???

area: 32

area_offset: 0x00611000

area_size: 0x000ef000 (978944)

area_name: GBB

&gt;&gt;&gt; Google Binary Block. Stores key information, FW screen bitmaps, and
special developer flags that can be set to modify boot behavior.

area: 33

area_offset: 0x00700000

area_size: 0x00100000 (1048576)

area_name: BOOT_STUB

&gt;&gt;&gt; RO Coreboot - Ramstage, romstage, CBFS, etc.
