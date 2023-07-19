# AVIF-info

**libavifinfo** is a standalone library that can be used to extract the width,
height, bit depth and number of channels from an AVIF payload.

See `avifinfo.h` for details on the API and `avifinfo.c` for the implementation.
See `avifinfo_test.cc` for usage examples.

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

```
AvifInfoFeatures features;
if (AvifInfoGet(bytes, number_of_available_bytes, &features) == kAvifInfoOk) {
  // Use 'features.width' etc.
}
```

Note: `AvifInfoGet()` is designed to return the same `avifImage` field values as
[`avifDecoderRead()`](https://github.com/AOMediaCodec/libavif/blob/9d8f9f9eb24fcea36113c946fa72f9f92aa7b317/include/avif/avif.h#L894).
However **libavifinfo** is more permissive and may return features of images
considered invalid by **libavif**.

### Build {#build}

`avifinfo.c` is written in C. To build from this directory:

```
mkdir build && \
cd build && \
cmake .. && \
cmake --build . --config Release
```

### Test {#test}

Tests are written in C++. GoogleTest is required.

```
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

`libavifinfo` was [implemented](https://github.com/php/php-src/pull/7711) into
**php-src** natively and is available through `getimagesize()` at head. If it is
not available in the PHP release version you use, you can fallback to
`avifinfo.php` instead.

See `avifinfo_test.php` for a usage example.

## Bug reports {#bug-reports}

Bug reports can be filed in the Alliance for Open Media
[issue tracker](https://bugs.chromium.org/p/aomedia/issues/list).
