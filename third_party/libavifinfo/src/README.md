# AVIF-info

**libavifinfo** is a standalone library that can be used to extract the width,
height, bit depth, number of channels and other metadata from an AVIF payload.

See `avifinfo.h` for details on the API and `avifinfo.c` for the implementation.
See `tests/avifinfo_demo.cc` for API usage examples.

## Contents

1.  [How to use](#how-to-use)

    1.  [Build](#build)
    2.  [Test](#test)

2.  [Development](#development)

    1.  [Coding style](#coding-style)
    2.  [Submitting patches](#submitting-patches)

3.  [PHP implementation](#php-implementation)

4.  [Bug reports](#bug-reports)

## How to use {#how-to-use}

**libavifinfo** can be used when only a few AVIF features are needed and when
linking to or including [libavif](https://github.com/AOMediaCodec/libavif) is
not an option. For decoding an image or extracting more features, please rely on
[libavif](https://github.com/AOMediaCodec/libavif).

Note: `AvifInfoGetFeatures()` is designed to return the same `avifImage` field
values as
[`avifDecoderParse()`](https://github.com/AOMediaCodec/libavif/blob/e34204f5370509c72b3b2f065e5ebb2767cbbd48/include/avif/avif.h#L1049).
However **libavifinfo** is more permissive and may return features of images
considered invalid by **libavif**.

### Build {#build}

`avifinfo.c` is written in C. To build from this directory:

```sh
mkdir build && \
cd build && \
cmake .. && \
cmake --build . --config Release
```

### Test {#test}

Tests are written in C++. GoogleTest is required.

```sh
mkdir build && \
cd build && \
cmake .. -DAVIFINFO_BUILD_TESTS=ON && \
cmake --build . --config Debug && \
ctest .
```

## Development {#development}

### Coding style {#coding-style}

[Google C/C++ Style Guide](https://google.github.io/styleguide/cppguide.html) is
used in this project.

### Submitting patches {#submitting-patches}

If you would like to contribute to **libavifinfo**, please follow the steps for
**libaom** at https://aomedia.googlesource.com/aom/#submitting-patches.

## PHP implementation

The PHP implementation of libavifinfo is a subset of the C API.

`libavifinfo` was [implemented](https://github.com/php/php-src/pull/7711) into
**php-src** natively and is available through `getimagesize()` at head. If it is
not available in the PHP release version you use, you can fallback to
`avifinfo.php` instead.

See `avifinfo_test.php` for a usage example.

## Bug reports {#bug-reports}

Bug reports can be filed in the Alliance for Open Media
[issue tracker](https://bugs.chromium.org/p/aomedia/issues/list).
