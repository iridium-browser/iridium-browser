# Cross Compilation

Open Screen Library supports cross compilation for both arm32 and arm64
platforms, by using the `gn args` parameter `target_cpu="arm"` or
`target_cpu="arm64"` respectively. Note that quotes are required around the
target arch value.

# How the Sysroots Work

Setting an arm/arm64 target_cpu causes GN to download a sysroot image from Open
Screen's public cloud storage bucket. This image is referenced by GN when
building using the compiler `--sysroot` flag. This allows the compiler to
effectively cross compile for different architectures.

# Updating the Sysroots

Unfortunately, only Google employees can update the sysroots to a new version,
by requesting access to the Open Screen pantheon project and uploading a new
tar.xz to the openscreen-sysroots bucket.

This documentation is available internally at
[go/openscreen-cross-compilation-doc](https://go/openscreen-cross-compilation-doc).

# Testing on an Embedded Device

To run executables on an embedded device, after copying the relevant binaries
you may need to install additional dependencies, such as `libavcodec` and
`libsdl`. For a list of these packages specifically for cast streaming, see the
[external libraries](../cast/streaming/external_libraries.md) documentation.
