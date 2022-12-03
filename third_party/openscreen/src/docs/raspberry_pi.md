# Working with ARM/ARM64/the Raspberry PI

Open Screen Library supports cross compilation for both arm32 and arm64
platforms, by using the `gn args` parameter `target_cpu="arm"` or
`target_cpu="arm64"` respectively. Note that quotes are required around the
target arch value.

Setting an arm(64) target_cpu causes GN to pull down a sysroot from openscreen's
public cloud storage bucket. Google employees may update the sysroots stored
by requesting access to the Open Screen pantheon project and uploading a new
tar.xz to the openscreen-sysroots bucket.

NOTE: The "arm" image is taken from Chromium's debian arm image, however it has
been manually patched to include support for libavcodec and libsdl2. To update
this image, the new image must be manually patched to include the necessary
header and library dependencies. Note that if the versions of libavcodec and
libsdl2 are too out of sync from the copies in the sysroot, compilation will
succeed, but you may experience issues decoding content.

To install the last known good version of the libavcodec and libsdl packages
on a Raspberry Pi, you can run the following command:

```bash
sudo ./cast/standalone_receiver/install_demo_deps_raspian.sh
```
