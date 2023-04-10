---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/ec-development
  - Chromium Embedded Controller (EC) Development
page_name: ec-image-geometry-spec
title: EC Image Geometry Spec
---

**Introduction**

The EC codebase currently supports the following chips:

lm4, stm32: Internal memory-mapped flash storage

cr50: Internal memory-mapped flash storage, with image signature preceding RO
image

npcx: External memory-mapped flash storage dedicated for EC, code copied to SRAM
before execution

mec1322: External unmapped flash storage dedicated for EC, code is copied to
SRAM before execution

For memory-mapped flash storage, contents are read from a chip-defined region in
memory. Contents are written through SPI commands. For unmapped flash storage,
contents are read and written through SPI commands.

There is currently no support in the codebase for shared external storage, but
we’d like to support it in the near future.

With three different storage categories supported, there is a lack of uniformity
within our code base. Certain geometry CONFIGs (ex. CONFIG_FLASH_BASE, which
originated when we had only internal memory-mapped flash) no longer have
meanings consistent with names. In other cases (ex. CONFIG_CDRAM_BASE) the
meaning of CONFIGs are too narrow and can be more broadly defined among all
three categories. In addition, a few CONFIGs (ex. CONFIG_RO_MEM_OFF) no longer
have any consistent meaning at all. This discontinuity makes it extremely
difficult to change higher-level code without breaking one or more chip
categories, or writing separate code to accommodate corner-cases for each
category.

The goal of the spec described here is to make our CONFIGs uniform and
understandable among all chip categories.

*   CONFIGs will be defined and clearly described here for programmer
            reference.
*   Modifying higher-level code that touches image geometry will become
            much less painful since all CONFIGs will have clear and consistent
            meaning.
*   Bringing up new chips will be simplified since our clear and
            consistent CONFIGs can be followed.

# Supported Configurations

With the changes proposed here, we aim to support ECs with the following
configurations:

*   Internal mapped storage
*   External mapped storage (shared or dedicated)
*   External unmapped storage (shared or dedicated)
*   Code executed directly from mapped storage
*   Code copied to SRAM before use

*   One RO image
*   One contiguous region of storage belonging to the EC, containing the
            RO image, that can be write protected
*   Up to one RW image
*   Up to one contiguous region of storage belonging to the EC,
            containing the RW image, that will not be write protected
*   Support for any number of chip-specific non-image data pieces
            (*NIDs*) as part of the storage regions (loaders, headers, etc - any
            piece of data on EC storage that isn’t part of the RO or RW image)
*   Write protected and writable EC storage regions need not be mutually
            contiguous

# Chip Definition CONFIGs

[<img alt="image"
src="/chromium-os/ec-development/ec-image-geometry-spec/storage2.png">](/chromium-os/ec-development/ec-image-geometry-spec/storage2.png)

The following CONFIGs are defined entirely based upon chip architecture with no
flexibility to change them based upon image / loader arrangement.

CONFIG_INTERNAL_STORAGE - Indicates that chip uses internal storage

CONFIG_EXTERNAL_STORAGE - Indicates that chip uses external storage

CONFIG_MAPPED_STORAGE - Indicates that storage is mapped into the EC's address
space

CONFIG_CDRAM_ARCH and CONFIG_FLASH_MAPPED previously encompassed these defines.

CONFIG_PROGRAM_MEMORY_BASE - Base address of program memory

CONFIG_MAPPED_STORAGE_BASE - Base address of memory-mapped flash storage (if
present)

CONFIG_FLASH_BASE and CONFIG_CDRAM_BASE previously encompassed these defines.

Examples:

lm4 will define CONFIG_INTERNAL_STORAGE and CONFIG_MAPPED_STORAGE, and set
CONFIG_PROGRAM_MEMORY_BASE = CONFIG_MAPPED_STORAGE_BASE = 0.

```none
/* Internal, mapped storage */#define CONFIG_INTERNAL_STORAGE#define CONFIG_MAPPED_STORAGE/* Program is directly run from storage */#define CONFIG_PROGRAM_MEMORY_BASE 0#define CONFIG_MAPPED_STORAGE_BASE 0
```

cr50 will define CONFIG_INTERNAL_STORAGE and CONFIG_MAPPED_STORAGE, and set
CONFIG_PROGRAM_MEMORY_BASE = CONFIG_MAPPED_STORAGE_BASE = 0x40000.

```none
/* Internal, mapped storage */#define CONFIG_INTERNAL_STORAGE#define CONFIG_MAPPED_STORAGE/* Program is directly run from storage */#define CONFIG_PROGRAM_MEMORY_BASE 0x40000#define CONFIG_MAPPED_STORAGE_BASE 0x40000
```

npcx will define CONFIG_EXTERNAL_STORAGE and CONFIG_MAPPED_STORAGE, and set
CONFIG_PROGRAM_MEMORY_BASE = 0x10088000, CONFIG_MAPPED_STORAGE_BASE =
0x64000000.

```none
/* External, mapped storage */#define CONFIG_EXTERNAL_STORAGE#define CONFIG_MAPPED_STORAGE/* Storage is memory-mapped, but program runs from SRAM */#define CONFIG_PROGRAM_MEMORY_BASE 0x10088000#define CONFIG_MAPPED_STORAGE_BASE 0x64000000
```

mec1322 will define CONFIG_EXTERNAL_STORAGE, and set CONFIG_PROGRAM_MEMORY_BASE
= 0x100000. CONFIG_MAPPED_STORAGE_BASE will not be defined, since storage is not
memory mapped.

```none
/* External storage */#define CONFIG_EXTERNAL_STORAGE/* Program memory in SRAM */#define CONFIG_PROGRAM_MEMORY_BASE 0x100000/* Storage is not memory mapped */#undef CONFIG_MAPPED_STORAGE#undef CONFIG_MAPPED_STORAGE_BASE
```

Layout / Geometry Configs

The following configs represent the Chrome EC implementation for a given chip
and will normally be defined at the chip-level. There is flexibility to change
these configs based upon needs. For example, the RO image on storage could
reside at the beginning or the end of storage, based upon protection
capabilities. Image sizes could grow or shrink if program memory space is
dedicated to a loader.

Since external storage may be shared between the EC and the AP, only a certain
region of storage may “belong” to the EC.

-------------------------------------

Color coding key:

Relative to address zero / beginning of external storage.

Relative to CONFIG_PROGRAM_MEMORY_BASE.

Relative to CONFIG_PROTECTED_STORAGE_OFF.

Relative to CONFIG_WRITABLE_STORAGE_OFF.

-------------------------------------

CONFIG_EC_PROTECTED_STORAGE_OFF - Offset of protected region of storage
belonging to the EC.

CONFIG_EC_PROTECTED_STORAGE_SIZE - Size of protected region of storage belonging
to the EC.

CONFIG_EC_WRITABLE_STORAGE_OFF - Offset of the writable region of storage (the
region that will be field auto-updated, when write protection is enabled)
belonging to the EC.

CONFIG_EC_WRITABLE_STORAGE_SIZE - Size of the writable region of storage
belonging to the EC.

CONFIG_FLASH_BASE_SPI was previously used to indicate the offset to storage
region belonging to EC. We now need multiple defines, because we can no longer
assume that there is one contiguous region of storage that belongs to the EC.

CONFIG_RO_MEM_OFF - RO image offset in program memory, when loaded.

CONFIG_RO_STORAGE_OFF - RO image offset on storage, relative to the start of the
EC protected storage region.

CONFIG_RO_SIZE - Size of RO image (in both program memory and storage).

CONFIG_RW_MEM_OFF - RW image offset in program memory, when loaded.

CONFIG_RW_B_MEM_OFF - RW B image (optional second identical RW image) offset in
program memory, when loaded. Only valid when CONFIG_RW_B is defined.

CONFIG_RW_STORAGE_OFF - RW image offset on storage, relative to the start of the
EC writable storage region.

CONFIG_RW_SIZE - Size of RW image (in both program memory and storage).

CONFIG_WP_STORAGE_OFF - Start of write protect region (the region that will be
write protected, when write protection is enabled) on EC storage. This may
exceed CONFIG_EC_PROTECTED_STORAGE_OFF if more than just the region belonging to
the EC must be protected.

CONFIG_WP_STORAGE_SIZE - Size of write protect region on EC storage. This may be
greater than CONFIG_EC_PROTECTED_STORAGE_SIZE for the same reason.

CONFIG_FW_R\*_OFF, CONFIG_FW_R\*_SPI_OFF and CONFIG_FW_R\*_SIZE previously
encompassed these defines.

Example:

<img alt="lm4.png"
src="https://lh3.googleusercontent.com/GtIICh7nWQFutGqAKRwZPOHKW15XMxUcj0OxydmlFDgwHPG-sp1tORd5aa2jDqccqTVoYMmB4pvPeHILgNdETa-Ayle6aBcCJk5TRmTair12eYUIKokrlKwCI_H8g4xf3HhuzYI"
height=317px; width=507px;>

lm4:

```none
/* Protected region is first half, followed by writable */#define CONFIG_EC_PROTECTED_STORAGE_OFF 0#define CONFIG_EC_PROTECTED_STORAGE_SIZE 0x20000#define CONFIG_EC_WRITABLE_STORAGE_OFF 0x20000#define CONFIG_EC_WRITABLE_STORAGE_SIZE 0x20000/* RO image fills protected region, less pstate at end */#define CONFIG_RO_MEM_OFF 0#define CONFIG_RO_STORAGE_OFF 0#define CONFIG_RO_SIZE (0x20000 - PSTATE_SIZE)/* RW image fills writable region */#define CONFIG_RW_MEM_OFF 0x20000#define CONFIG_RW_STORAGE_OFF 0#define CONFIG_RW_SIZE 0x20000/* Protect first half of storage: RO image + pstate */#define CONFIG_WP_STORAGE_OFF 0#define CONFIG_WP_STORAGE_SIZE 0x20000
```

[<img alt="image"
src="/chromium-os/ec-development/ec-image-geometry-spec/cr50.png">](/chromium-os/ec-development/ec-image-geometry-spec/cr50.png)

cr50:

```none
/* Protected region is first half, followed by writable */#define CONFIG_EC_PROTECTED_STORAGE_OFF 0#define CONFIG_EC_PROTECTED_STORAGE_SIZE 0x20000#define CONFIG_EC_WRITABLE_STORAGE_OFF 0x20000#define CONFIG_EC_WRITABLE_STORAGE_SIZE 0x20000/* Signature is first 1024 bytes of protected region */#define SIG_STORAGE_OFF 0 /* Not a globally defined config */#define SIG_SIZE 1024 /* Not a globally defined config *//* RO image fills remainder protected region, less pstate at end */#define CONFIG_RO_MEM_OFF SIG_SIZE#define CONFIG_RO_STORAGE_OFF SIG_SIZE#define CONFIG_RO_SIZE (0x20000 - SIG_SIZE - PSTATE_SIZE)/* RW image fills writable region */#define CONFIG_RW_MEM_OFF 0x20000#define CONFIG_RW_STORAGE_OFF 0#define CONFIG_RW_SIZE 0x20000/* Protect first half of storage: sig + RO image + pstate */#define CONFIG_WP_STORAGE_OFF CONFIG_EC_PROTECTED_STORAGE_OFF#define CONFIG_WP_STORAGE_SIZE CONFIG_EC_PROTECTED_STORAGE_SIZE
```

--------------------------------------------

<img alt="mec1322_storage.png"
src="https://lh4.googleusercontent.com/nXQXaXn8UaC_gUB-JGk_vuje4hwOevjlnu1M5jhzltfsYyE0hvN_3xKRSBT-SNL_smeF-IB6LgEpbjU2bXvHgSusY666rHp5IS70KYWRLblQ2F0RFzJFEsxyq2G1tTI2rB3Sr2U"
height=273px; width=579px;>

<img alt="mec1322_program_memory.png"
src="https://lh6.googleusercontent.com/dOVZvS9j3m23lCmKvRPMoAUpDx9X0WWj5i4X4sx9tVFUHLbsxJypKpwms-4kehSnnJxI63s2aKMYxoekijpP1eL3w9Ux6ZwhFoksdvj0qbOHh3JUlZO2t9FMdeMg7pKKKZklRLc"
height=236px; width=513px;>

mec1322 (cyan):

```none
/* External unmapped storage, only second half belongs to EC *//* RW EC region followed by RO EC region */#define EC_STORAGE_START 0x40000 /* Not a globally defined config */#define CONFIG_EC_PROTECTED_STORAGE_OFF EC_STORAGE_START + 0x20000#define CONFIG_EC_PROTECTED_STORAGE_SIZE 0x20000#define CONFIG_EC_WRITABLE_STORAGE_OFF EC_STORAGE_START#define CONFIG_EC_WRITABLE_STORAGE_SIZE 0x20000/* Loader lives at beginning of program memory */#define CONFIG_RO_MEM_OFF (0 + LOADER_SIZE)/* RO image resides after loader in protected EC storage region */#define CONFIG_RO_STORAGE_OFF (0 + LOADER_SIZE)/* Image size is 96k minus loader, bounded by program memory */#define CONFIG_RO_SIZE (1024 * 96 - LOADER_SIZE)/* RW image lives at same program memory location as RO */#define CONFIG_RW_MEM_OFF (0 + LOADER_SIZE)/* RW image resides at beginning of writable EC storage region */#define CONFIG_RW_STORAGE_OFF 0#define CONFIG_RW_SIZE (1024 * 96 - LOADER_SIZE)/* WP region corresponds to EC protected storage region */#define CONFIG_WP_STORAGE_OFF CONFIG_EC_PROTECTED_STORAGE_OFF#define CONFIG_WP_STORAGE_SIZE CONFIG_EC_PROTECTED_STORAGE_SIZE
```

--------------------------------------------

<img alt="image"
src="https://lh4.googleusercontent.com/NuXt7Tq-8pzO_pLLyZ-_Drf1VbRnimdPBKuFJGvoBwUorNeqgZ7NPUIl-SbqXqeQaYqsyChFn3xw6_YmO5fpdiXXus_4b1ibKn6JuhGnfRH3GSb33IBhgqs-YJdWWxh7LB3QeYE"
height=167px; width=553px;>

<img alt="image"
src="https://lh5.googleusercontent.com/EuHdkHp1XBXK1yCWL9ogJrZJkXjzLrK3tLORPPXP3CWFezWL3t8B4l1SnA24Hvtx4a-MFHsJPnL_yQJd5OiKzqMTSppt8wpRpMRpGRsdKmwSdEbnO_-YJv7MfP6FT9T0qvZtk0Y"
height=139px; width=342px;>

npcx (npcx_evb):

```none
/* External mapped storage, only first half belongs to EC *//* RO EC region followed by RW EC region */#define EC_STORAGE_START 0 #define CONFIG_EC_PROTECTED_STORAGE_OFF EC_STORAGE_START#define CONFIG_EC_PROTECTED_STORAGE_SIZE 0x20000#define CONFIG_EC_WRITABLE_STORAGE_OFF EC_STORAGE_START + 0x20000#define CONFIG_EC_WRITABLE_STORAGE_SIZE 0x20000/* RO image will reside at beginning of program memory */#define CONFIG_RO_MEM_OFF 0/* RO image resides after loader in protected EC storage region */#define CONFIG_RO_STORAGE_OFF (0 + CONFIG_BOOTHEADER_SIZE)/* Image size is 96k minus header, bounded by program memory */#define CONFIG_RO_SIZE (1024 * 96 - CONFIG_BOOTHEADER_SIZE)/* RW image lives at same program memory location as RO */#define CONFIG_RW_MEM_OFF 0/* RW image resides at beginning of writable EC storage region */#define CONFIG_RW_STORAGE_OFF 0#define CONFIG_RW_SIZE (1024 * 96)/* WP region corresponds to EC protected storage region */#define CONFIG_WP_STORAGE_OFF CONFIG_EC_PROTECTED_STORAGE_OFF#define CONFIG_WP_STORAGE_SIZE CONFIG_EC_PROTECTED_STORAGE_SIZE
```

--------------------------------------------<img alt="SharedSpi.png"
src="https://lh4.googleusercontent.com/qtVOKTuf4Irw2bpitHmMFzINVKkA54Xfsmb6terAPANSs0MmUUeVQraK3zWbhG_HG6ZgyvVBaEs9I-o3GItjGAceqpz_LOv0ksMxIlME9_F___QJZr5kWxhQPYEQm8ohBNH-2O4"
height=360px; width=624px;>Hypothetical shared SPI platform (see Appendix):

```none
/* Non-contiguous RO + RW images on storage */#define CONFIG_EC_PROTECTED_STORAGE_OFF 0x7e0000#define CONFIG_EC_PROTECTED_STORAGE_SIZE 0x20000#define CONFIG_EC_WRITABLE_STORAGE_OFF 0x1e0000#define CONFIG_EC_WRITABLE_STORAGE_SIZE 0x20000...#define CONFIG_RO_STORAGE_OFF (0 + LOADER_SIZE)#define CONFIG_RW_STORAGE_OFF 0.../* Protected region includes non-EC data */#define CONFIG_WP_STORAGE_OFF 0x600000#define CONFIG_PROTECTED_SIZE 0x200000
```

# Handling Non-Image Data Pieces (NIDs)

Different chips may have different NIDs, such as boot headers and bootloaders.
Since these NIDs may have chip-specific requirements, we will not attempt to
standardize their layouts. It is suggested to keep uniform naming for NID
geometry configs, such as:

```none
#define CONFIG_BOOTLOADER_OFF#define CONFIG_BOOTLOADER_SIZE#define CONFIG_BOOTHEADER_OFF#define CONFIG_BOOTHEADER_SIZE
```

These chip-specific NID-related CONFIGs may be used in chip folders as required.
They should **NEVER** be used outside of chip folders (except to describe them
in config.h) since their precise meanings are not standardized.

# Linking

Our linker scripts were originally written to support internal memory-mapped
storage with only RO and RW images (no NIDs). The scripts will be extended to
support the new configs defined above. For example, the linker will produce a
unified image with the RW image at the appropriate offset, as defined by
CONFIG_RW_STORAGE_OFF / CONFIG_RW_STORAGE_SIZE.

If NIDs are required, they will need to be copied to the appropriate location by
a chip-specific packing script. Such a script has already been implemented for
mec1322 in chip/mec1322/util/pack_ec.py. We may consider implementing a generic
version of pack_ec.py in the future, with only custom parameters for each chip.

NIDs should not be copied by modifying the global, non-chip linker scripts. For
example, the NPCX-custom changes related to NPCX_RO_HEADER in
core/cortex-m/ec.lds.S are not acceptable and will need to be removed.

# FMAP

FMAP is a data structure (authoritative copy today lives inside the RO image,
but copies exist in both images) that describes the layout of EC storage.

Its header defines the following values of significance:

fmap_base

fmap_size

In addition, boundaries for seven regions are defined.

Currently, CONFIG_PROGRAM_MEMORY_BASE is written to fmap_base, and
CONFIG_EC_STORAGE_SIZE is written to fmap_size. This design makes no sense for
an FMAP intended to be read outside of the EC, since CONFIG_PROGRAM_MEMORY_BASE
is only meaningful to the EC. Instead, the following changes are proposed:

fmap_base: Offset to storage region described by FMAP

fmap_size: Size of storage region described by FMAP

When protected and writable storage regions are contiguous, the FMAP in RO and
RW (if present) images will be identical and describe both regions, with the RO
copy being authoritative. This is entirely consistent with the way FMAP works
today (all current platforms have contiguous readable + writable storage
regions).

## Shared SPI

When protected and writable storage regions are non-contiguous (in certain
future shared SPI designs), the FMAP in RO and RW images will be different and
will each describe the EC protected and writable storage regions, respectfully.

For shared SPI, there will also be *master FMAP* (today known as the FW SPI
FMAP), outside the EC storage regions,that describes all regions of SPI storage,
including FW romstage images, ramstage images, vendor blobs, and EC storage
regions.

The workings of the *master FMAP* are outside the scope of the document, but
it’s anticipated that EC FW updates could work as follows:

The *master FMAP* has separate entries for the EC RO update region, the EC RW
update region, and the SPI write protect region. Therefore, the EC regions can
be flashed without parsing the EC FMAP. If desired, the EC FMAP can be parsed to
ensure consistency before flashing.

## FMAP Regions

The seven regions are unchanged and will keep their overall meaning (though some
corrections still must be made in code). Note that all region start addresses
are relative to fmap_base.

EC_RO - Region that will be updated on an RO update (equivalent to the
CONFIG_EC_PROTECTED_STORAGE region). This region includes the RO image and any
other meaningful NIDs in the RO region. Note that RO FW update is a feature not
supported on production devices, yet it is important for pre-MP updates and
factory flow.

FR_MAIN - Region consisting only of RO image (CONFIG_RO_STORAGE region)

RO_FRID - Region consisting of version struct inside RO image (generally start
of FR_MAIN + constant offset)

FMAP - Region consisting of the FMAP inside RO image, or the FMAP inside the RW
image, if the RO image doesn’t exist.

WP_RO - EC region that will be write protected at the factory before shipping
PVT / production devices. Equivalent to EC_RO. Note that if the protect region
exceeds the fmap_base / fmap_size region (when the CONFIG_WP_STORAGE exceeds the
CONFIG_EC_PROTECTED_STORAGE region, such as with shared SPI) then WP_RO doesn’t
describe the entire SPI region that must be protected. The *master FMAP* must
describe that region separately.

EC_RW - Region that will be updated on an RW update (CONFIG_EC_WRITABLE_STORAGE
region).

RW_FWID - Region consisting of version struct inside RW image.

FR_MAIN (EC RO image) &lt;= EC_RO (RO update region /
CONFIG_EC_PROTECTED_STORAGE region) = WP_RO (EC write protect region)

EC RW IMAGE (not defined by FMAP) &lt;= EC_RW (RW update region /
CONFIG_WRITABLE_STORAGE region)

EC_RO + EC_RW == fmap_base + fmap_size, for contiguous EC_RO + EC_RW

# Appendix

## Hypothetical Shared SPI Platform

*   Uses hypothetical MEC1322 (actual shared SPI implementation details
            not consulted)
*   8 MB (0x800000) SPI ROM
*   EC writable region 128K resides at \[0x1e0000, 0x200000)
*   EC protected region 128K resides at \[0x7e0000, 0x800000)
*   Remaining non-EC regions belong to system FW / BIOS
*   SPI protected region 2MB resides at \[0x600000, 0x800000)
