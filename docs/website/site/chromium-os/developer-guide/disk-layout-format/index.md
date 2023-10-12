---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: disk-layout-format
title: Disk Layout Format
---

Boards can define their own disk layout, with different partitioning ordering
and sizes.

A board may specify its own layout by placing a layout file in
overlay-&lt;board&gt;/scripts/disk_layout.json; if no layout file is found, the
default one in scripts/build_library/legacy_disk_layout.json will be used.

A layout file consists of a "base" layout, plus zero or more other types of
layout. Common types would be "usb" and "vm". Each of these layouts is made up
of one or more partitions.

The base layout is special, since it's the layout on which all other layouts get
overlaid upon. The base layout represents the "installed" layout, the way things
should be once the image has been installed onto a device. The "usb" layout
represents how things should look on a USB image (typically, this means that
ROOT-B is 1 block long).

See
<https://chromium.googlesource.com/chromiumos/platform/crosutils/+/HEAD/build_library/README.disk_layout>
for the exact format details as they may change over time.

Here's an example of a partition entry in a layout:

{

"num": 5,

"label":"ROOT-B",

"type":"rootfs",

"blocks":"4194304",

"fs_blocks":"217600"

}

The "num" field defines the numerical ID of the partition in the GPT. The
"label" field defines the name of the partition in the GPT table and also the
label of the filesystem if the given partition has a filesystem. The "blocks"
field defines the number of blocks in size the partition is. Optionally, you can
also specify the "fs_blocks" field. This defines how large the filesystem on the
given partition should be. If you don't specify "fs_blocks", the entire
partition is used.

The block size for a partition and a filesystem is defined at the top of the
layout file, in the metadata section:

"metadata":{

"block_size": 512,

"fs_block_size": 4096

},

Both of these values should be specified in bytes.

**FAQ**

*   I want to use gmerge and I used to use the --rootfs_size and
            --rootfs_partition_size options to make my root filesystem larger to
            aid this, how do I do this now?
    *   Open up legacy_disk_layout.json and edit the ROOT-A/ROOT-B
                blocks and fs_blocks to your desired sizes, using the block
                sizes specified at the top of the file. Then copy the values for
                ROOT-A into the the blocks/fs_blocks for the usb layout. If you
                want to do this for a VM, copy the values into the vm layout
                too.
