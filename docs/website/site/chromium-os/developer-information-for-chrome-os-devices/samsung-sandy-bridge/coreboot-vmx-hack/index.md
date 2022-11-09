---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
- - /chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge
  - Samsung Series 5 550 Chromebook and Series 3 Chromebox
page_name: coreboot-vmx-hack
title: Hacking VMX Support Into Coreboot (lumpy & stumpy)
---

[TOC]

**If you are not using Samsung 5 550 (lumpy) or a Samsung 3 Chromebox (stumpy), back away slowly now. These instructions WILL NOT WORK for any other device.**
**This document is intended for people who don't mind getting their hands dirty,
voiding warranties, or possibly bricking their devices. If any of those thing
scare you, just walk away.**

## Background

The BIOS that shipped on earlier devices (such as stumpy and lumpy) had minor
misfeatures in them where they would disable VMX support in the read-only
firmware stage. That means there is no way to use those virtualization
extensions once you booted. Putting into dev mode won't help, nor will
installing an alternative OS.

This document attempts to guide you through the process of binary patching your
BIOS. This way VMX support won't be disabled while it is loaded. Instead, the
kernel itself will take care of enabling/disabling it. This way you can leverage
that hardware functionality to use KVM or other such technologies. The [Virtual
Machine](/chromium-os/developer-information-for-chrome-os-devices/running-virtual-machines-on-your-chromebook)
explains more.

If you're still here, and you have a lumpy or stumpy device, and you want to
hack your BIOS (coreboot) to keep VMX (hardware virtualization insns) support
from being disabled at boot, then let's get into it.

## Prepare The Device

Make sure your device is in dev mode already and you can get a root shell. See
the [Samsung Series 5 550 Chromebook and Series 3
Chromebox](/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge)
page for details on how to do this.

You should also see what version of firmware you're running. All the example
output below is taken from a Chromebox. This will be invaluable if you ask for
assistance.

```none
$ crossystem | grep fwidfwid                   = Google_Stumpy.2.102.0          # Active firmware IDro_fwid                = Google_Stumpy.2.102.0          # Read-only firmware ID
```

Note that while most of these commands can be run on the device, some will
require tools that are available in the Chromium OS SDK (like cbfstool and a
compiler/assembler). So you should extract the bios on the device, and then do
all the other commands on your development system. You can transfer the file via
scp.

## Extracting The BIOS

Now that you're in dev mode, extract the BIOS from the read-only SPI flash using
flashrom.

```none
# Run this on the device.$ cd /tmp$ PATH+=:/sbin:/usr/sbin:/usr/local/sbin$ sudo flashrom -r bios.bin# Now scp the bios.bin off the device and do follow up commands# in your ChromiumOS sdk chroot.
```

The BIOS can be read with fmap_decode.

```none
$ fmap_decode bios.binfmap_signature="0x5f5f464d41505f5f" fmap_ver_major="1" fmap_ver_minor="0" fmap_base="0x0000000000000000" fmap_size="0x800000" fmap_name="FMAP" fmap_nareas="27"area_offset="0x00000000" area_size="0x00180000" area_name="SI_ALL" area_flags_raw="0x01" area_flags="static"area_offset="0x00000000" area_size="0x00001000" area_name="SI_DESC" area_flags_raw="0x01" area_flags="static"area_offset="0x00001000" area_size="0x0017f000" area_name="SI_ME" area_flags_raw="0x01" area_flags="static"area_offset="0x00180000" area_size="0x00680000" area_name="SI_BIOS" area_flags_raw="0x01" area_flags="static"area_offset="0x00180000" area_size="0x00001000" area_name="RW_VPD" area_flags_raw="0x01" area_flags="static"area_offset="0x00181000" area_size="0x00067000" area_name="RW_UNUSED" area_flags_raw="0x01" area_flags="static"area_offset="0x001e8000" area_size="0x00018000" area_name="RW_SHARED" area_flags_raw="0x01" area_flags="static"area_offset="0x001e8000" area_size="0x00004000" area_name="RW_ENVIRONMENT" area_flags_raw="0x01" area_flags="static"area_offset="0x001ec000" area_size="0x00010000" area_name="RW_MRC_CACHE" area_flags_raw="0x01" area_flags="static"area_offset="0x001fc000" area_size="0x00004000" area_name="DEV_CFG" area_flags_raw="0x01" area_flags="static"area_offset="0x00200000" area_size="0x00100000" area_name="RW_SECTION_A" area_flags_raw="0x01" area_flags="static"area_offset="0x00200000" area_size="0x00010000" area_name="VBLOCK_A" area_flags_raw="0x01" area_flags="static"area_offset="0x00210000" area_size="0x000effc0" area_name="FW_MAIN_A" area_flags_raw="0x01" area_flags="static"area_offset="0x002fffc0" area_size="0x00000040" area_name="RW_FWID_A" area_flags_raw="0x01" area_flags="static"area_offset="0x00300000" area_size="0x00100000" area_name="RW_SECTION_B" area_flags_raw="0x01" area_flags="static"area_offset="0x00300000" area_size="0x00010000" area_name="VBLOCK_B" area_flags_raw="0x01" area_flags="static"area_offset="0x00310000" area_size="0x000effc0" area_name="FW_MAIN_B" area_flags_raw="0x01" area_flags="static"area_offset="0x003fffc0" area_size="0x00000040" area_name="RW_FWID_B" area_flags_raw="0x01" area_flags="static"area_offset="0x00400000" area_size="0x00170000" area_name="RO_UNUSED_1" area_flags_raw="0x01" area_flags="static"area_offset="0x00570000" area_size="0x00020000" area_name="RO_VPD" area_flags_raw="0x01" area_flags="static"area_offset="0x00590000" area_size="0x000e0000" area_name="RO_UNUSED_2" area_flags_raw="0x01" area_flags="static"area_offset="0x00670000" area_size="0x00190000" area_name="RO_SECTION" area_flags_raw="0x01" area_flags="static"area_offset="0x00670000" area_size="0x00000800" area_name="FMAP" area_flags_raw="0x01" area_flags="static"area_offset="0x00670800" area_size="0x00000040" area_name="RO_FRID" area_flags_raw="0x01" area_flags="static"area_offset="0x00670840" area_size="0x0000f7c0" area_name="RO_PADDING" area_flags_raw="0x01" area_flags="static"area_offset="0x00680000" area_size="0x00080000" area_name="GBB" area_flags_raw="0x01" area_flags="static"area_offset="0x00700000" area_size="0x00100000" area_name="BOOT_STUB" area_flags_raw="0x01" area_flags="static"
```

Here we see all the wonderful sub-sections of the BIOS. The one we really care
about is the last part -- the BOOT_STUB.

### Extracting Coreboot

Using the details from the last line from the fmap_decode output, we can use dd
to extract coreboot.

```none
$ eval `fmap_decode bios.bin | grep BOOT_STUB`$ dd if=bios.bin ibs=$((area_offset)) skip=1 | dd bs=$((area_size)) iflag=fullblock of=coreboot.bin
```

You can verify this actually worked by running the cbfstool (part of coreboot,
and in the Chromium OS sdk chroot).

```none
$ cbfstool coreboot.bin printUpdating CBFS master header to version 2coreboot.bin: 1024 kB, bootblocksize 10498, romsize 1048576, offset 0x0Alignment: 64 bytes, architecture: x86Name                           Offset     Type         Sizecmos_layout.bin                0x0        cmos layout  1223pci8086,0106.rom               0x500      optionrom    65536fallback/romstage              0x10540    stage        34084fallback/coreboot_ram          0x18ac0    stage        79377fallback/payload               0x2c140    payload      106157u-boot.dtb                     0x46040    unknown      8144(empty)                        0x48040    null         622424mrc.bin                        0xdffc0    unknown      102924(empty)                        0xf9240    null         17526
```

#### Extracting Coreboot RAM Stage

We want the coreboot_ram stage there. Let's extract it using cbfstool.

```none
$ cbfstool coreboot.bin extract -n fallback/coreboot_ram -f coreboot_ram.lz Updating CBFS master header to version 2coreboot.bin: 1024 kB, bootblocksize 10498, romsize 1048576, offset 0x0Alignment: 64 bytesFound file fallback/coreboot_ram at 0x18ac0, type stage, size 79377Warning: only 'raw' files are safe to extract.Successfully dumped the file.
```

#### Decompressing Coreboot RAM Stage

You might have noticed we called the output file here coreboot_ram.lz instead of
just coreboot_ram. That's because the stage is compressed by LZMA. That means we
need to decompress it first.

```none
$ dd if=coreboot_ram.lz ibs=$((0x1c)) skip=1 | lzma -dc > coreboot_ram.bin
```

We skip 0x1c bytes there because that's the CBFS stage header information. This
should be constant across all images.

Now, finally, we have the raw x86 code that gets executed during initial boot.

## Hacking Coreboot

You can do a sanity check to make sure we've got the right file by using objdump
and disassembling the code.

```none
$ objdump -D -b binary -m i386 coreboot_ram.bincoreboot_ram.bin:     file format binaryDisassembly of section .data:00000000 <.data>:       0:   fa                      cli           1:   2e 0f 01 15 37 01 10    lgdtl  %cs:0x100137       8:   00        9:   ea 10 00 10 00 10 00    ljmp   $0x10,$0x100010      10:   b8 18 00 00 00          mov    $0x18,%eax      15:   8e d8                   mov    %eax,%ds      17:   8e c0                   mov    %eax,%es      19:   8e d0                   mov    %eax,%ss      1b:   8e e0                   mov    %eax,%fs      1d:   8e e8                   mov    %eax,%gs      1f:   b0 13                   mov    $0x13,%al      21:   e6 80                   out    %al,$0x80      23:   fc                      cld    ...
```

Your output might not look \*exactly\* like that (some of the number constants
might be different). The first few instruction names should be the same though
(a cli, then a lgdtl, then a ljmp, then a bunch of mov's). If your output
doesn't resemble this, then you should abort and seek help.

### Locate wrmsr

Buried in that output somewhere is the magic insn sequence that turns off VMX on
us. This is done by writing to the 0x3a machine specific register using the
wrmsr instruction. Let's scan the output for it.

The key sequence to look for is a mov insn that stores 0x3a in the eax register,
followed by a call insn to a small func that does a rdmsr+wrmsr, followed by
some more movs/calls and then a cpuid insn.

```none
$ objdump -D -b binary -m i386 coreboot_ram.bin | less...   26d4d:   57                      push   %edi   26d4e:   31 d2                   xor    %edx,%edx   26d50:   b8 3a 00 00 00          mov    $0x3a,%eax           <-- store of 0x3a into eax   26d55:   e8 ba ff ff ff          call   0x26d14              <-- call to local func (see below)   26d5a:   ba 0f 00 00 00          mov    $0xf,%edx            <-- two move insns   26d5f:   b8 e2 00 00 00          mov    $0xe2,%eax   26d64:   e8 ab ff ff ff          call   0x26d14              <-- call to same local func   26d69:   b8 01 00 00 00          mov    $0x1,%eax   26d6e:   89 df                   mov    %ebx,%edi   26d70:   0f a2                   cpuid                       <-- cpuid call   26d72:   89 fb                   mov    %edi,%ebx   26d74:   81 e1 00 00 00 02       and    $0x2000000,%ecx   26d7a:   74 0c                   je     0x26d88   26d7c:   31 d2                   xor    %edx,%edx   26d7e:   b8 3c 01 00 00          mov    $0x13c,%eax   26d83:   e8 8c ff ff ff          call   0x26d14              <-- more calls to same local func   26d88:   ba 1f 00 00 00          mov    $0x1f,%edx   26d8d:   b8 01 06 00 00          mov    $0x601,%eax   26d92:   e8 7d ff ff ff          call   0x26d14              <-- more calls to same local func...# here is the func that is being called (address matches)...   26d14:   55                      push   %ebp                 <-- function prologue   26d15:   89 c1                   mov    %eax,%ecx   26d17:   89 c5                   mov    %eax,%ebp   26d19:   57                      push   %edi   26d1a:   56                      push   %esi   26d1b:   89 d6                   mov    %edx,%esi   26d1d:   0f 32                   rdmsr                       <-- the rdmsr call   26d1f:   83 fe 1f                cmp    $0x1f,%esi   26d22:   77 11                   ja     0x26d35   26d24:   89 f1                   mov    %esi,%ecx   26d26:   bf 01 00 00 00          mov    $0x1,%edi   26d2b:   d3 e7                   shl    %cl,%edi   26d2d:   85 f8                   test   %edi,%eax   26d2f:   75 18                   jne    0x26d49   26d31:   09 f8                   or     %edi,%eax   26d33:   eb 10                   jmp    0x26d45   26d35:   8d 4e e0                lea    -0x20(%esi),%ecx   26d38:   bf 01 00 00 00          mov    $0x1,%edi   26d3d:   d3 e7                   shl    %cl,%edi   26d3f:   85 fa                   test   %edi,%edx   26d41:   75 06                   jne    0x26d49   26d43:   09 fa                   or     %edi,%edx   26d45:   89 e9                   mov    %ebp,%ecx   26d47:   0f 30                   wrmsr                       <-- the wrmsr call   26d49:   5e                      pop    %esi                 <-- function epilog   26d4a:   5f                      pop    %edi   26d4b:   5d                      pop    %ebp   26d4c:   c3                      ret...
```

Above you can see the "bad" call to the wrmsr function happens at offset
0x26d55.

### Nopping The wrmsr

We now have the right offset, so let's patch that particular insn to be a nop
instead! :)

```none
$ dd if=/dev/zero bs=5 count=1 | tr '\0' $'\x90' | dd conv=notrunc of=coreboot_ram.bin obs=$((0x26d55)) seek=1
```

This will write five 0x90 bytes (the hex for the nop insn) to the offset of the
call insn. Let's see our handy work by consulting objdump.

```none
$ objdump -D -b binary -m i386 coreboot_ram.bin | less...   26d4d:   57                      push   %edi   26d4e:   31 d2                   xor    %edx,%edx   26d50:   b8 3a 00 00 00          mov    $0x3a,%eax           <-- same 0x3a store into eax   26d55:   90                      nop   26d56:   90                      nop   26d57:   90                      nop                         <-- 5 nops where there was a call   26d58:   90                      nop   26d59:   90                      nop   26d5a:   ba 0f 00 00 00          mov    $0xf,%edx            <-- same mov insns as before   26d5f:   b8 e2 00 00 00          mov    $0xe2,%eax   26d64:   e8 ab ff ff ff          call   0x26d14...
```

You can see that we have successfully executed our gambit!

## Repacking The BIOS Image

We've broken everything down and made our small change. Let's rebundle
everything so we can deploy the fix on our device.

### Repacking Coreboot

We have to get creative now. It's easier if we just let the cbfstool repack the
code since it includes compressed data and checksums in the headers and other
fun stuff rather than trying to do it manually using dd.

#### Rebuilding Coreboot RAM Stage

First rebuild the input ELF using the modified binary code.

```none
$ data_size=`hexdump -v -e '"%#_ax %_u\n"' coreboot_ram.bin | awk '$NF != "nul" { a = $1 } END { print a }'`$ data_size=$(( (data_size + 0x1000) & ~(0x1000 - 1) ))$ mem_size=$(( `wc -c < coreboot_ram.bin` - data_size ))$ dd if=coreboot_ram.bin of=coreboot_ram.s.bin bs=$((data_size)) count=1$ printf '.incbin "coreboot_ram.s.bin"\n.comm b,%s+%s,1\n' ${mem_size} 0x44000 > coreboot_ram.s$ as --32 coreboot_ram.s -o coreboot_ram.o$ ld -m elf_i386 coreboot_ram.o -o coreboot_ram.elf -e 0x100000 -Ttext 0x100000
```

#### Updating Coreboot

Now delete the existing stage from coreboot, and then add the new one.

```none
$ cp coreboot.bin coreboot.bin.new$ cbfstool coreboot.bin.new remove -n fallback/coreboot_ram$ cbfstool coreboot.bin.new add-stage -f coreboot_ram.elf -n fallback/coreboot_ram -c lzma
```

If you look at the cbfstool print output, you might notice that the order of the
components has changed (the ram stage now comes later and there is an empty hole
where the ram stage used to be). That's not a problem -- coreboot is smart
enough to dynamically scan the CBFS structure for the ram stage.

### Updating The BIOS Image

This part is easiest to do with dd again.

```none
$ eval `fmap_decode bios.bin | grep BOOT_STUB`$ cp bios.bin bios.bin.new$ dd if=coreboot.bin.new of=bios.bin.new obs=$((area_offset)) seek=1 ibs=$((area_size)) count=1
```

## Flashing The New BIOS

This is the only part where things can go wrong and possibly brick your device.
Make sure your device is fully plugged in before attempting this process (like
the lumpy -- don't run it on battery).

The details of each step below can be found in the [Samsung Series 5 550
Chromebook and Series 3
Chromebox](/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge)
page. Consult that as you go.

### Hardware Preparation

1.  Open the case
    1.  You'll now have to disassemble your device (which most likely
                voids your warranty).
2.  Disable write protect
    1.  You'll have to locate the Write Protect jumper and enable it.
    2.  This will make the BIOS read-write so you can update it.
    3.  Once things have been updated, you can undo this so your BIOS is
                read-only again.
3.  Reassemble the device
    1.  Now that you've done toggled the jumper, you need to reassemble
                the device and power it up.
4.  Check the write protect
    1.  Run crossystem and look at the wpsw_cur field; it should be 0.
5.  Plug your system in to be safe!
    1.  For the Chromebook laptop, make sure the battery is charged, and
                the power supply is connected.
    2.  For the Chromebox, try and plug it into a battery backup (UPS),
                and don't try this during a storm :).

### Write The BIOS

With the new bios.bin.new file in hand, let's use flashrom to update things.
Obviously you'll need to scp this file back to the device and run flashrom
there.

```none
$ sudo flashrom -w bios.bin.new
```

Then read it back out to verify things worked

```none
$ sudo flashrom -r bios.bin.check$ md5sum bios.bin.new bios.new.check
```

If the md5 hashes match, then you're ready to cross your fingers and reboot!

If you saw any weird errors, then do not power off the device. Try and reprogram
the original BIOS instead.

```none
$ sudo flashrom -w bios.bin
```

### Check VMX Support

Now follow the normal [Virtual
Machine](/chromium-os/developer-information-for-chrome-os-devices/running-virtual-machines-on-your-chromebook)
document. You should be able to use kvm and other fun things on your device now.

Note that if the initial reboot worked, you actually have to power off the
device (not just reboot) in order for the relevant software registers to fully
reset themselves.

### Enable Write Protect

Now that you've made sure everything works, you should re-enable the write
protection on your BIOS. Follow the Hardware Preparation steps above to remove
the physical jumper.

You'll also have to run some software steps once you reboot. This tells the
flash to re-enable write protection on itself. We only want to do this for the
2nd half of the flash though and not the entire thing.

First get the current status. It should look something like:

<pre><code>$ sudo flashrom --wp-statusflashrom v0.9.4  : .............WP: status: 0x0000WP: status.srp0: 0WP: status.srp1: 0WP: write protect is <b>disabled</b>.WP: write protect range: <b>start=0x00000000, len=0x00000000</b>
</code></pre>

Now get the flash size:

<pre><code>$ sudo flashrom --get-sizeflashrom v0.9.4  : .............<b>8388608</b>
</code></pre>

Take that value and divide it by two and use that to re-enable write protection:

<pre><code>$ sudo flashrom --wp-range $((8388608/2)) $((8388608/2))flashrom v0.9.4  : .............<b>SUCCESS</b>$ sudo flashrom --wp-enableflashrom v0.9.4  : .............<b>SUCCESS</b>
</code></pre>

Finally double check the result:

<pre><code>$ sudo flashrom --wp-statusflashrom v0.9.4  : .............WP: status: 0x0098WP: status.srp0: 1WP: status.srp1: 0WP: write protect is <b>enabled</b>.WP: write protect range: <b>start=0x00400000, len=0x00400000</b>
</code></pre>
