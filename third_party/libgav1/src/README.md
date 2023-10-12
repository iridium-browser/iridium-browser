# libgav1 -- an AV1 decoder

libgav1 is a Main profile (0), High profile (1) & Professional profile (2)
compliant AV1 decoder. More information on the AV1 video format can be found at
[aomedia.org](https://aomedia.org).

[TOC]

## Building

### Prerequisites

1.  A C++11 compiler. gcc 6+, clang 7+ or Microsoft Visual Studio 2017+ are
    recommended.

2.  [CMake >= 3.7.1](https://cmake.org/download/)

3.  [Abseil](https://abseil.io)

    From within the libgav1 directory:

    ```shell
    $ git clone -b 20220623.0 --depth 1 \
      https://github.com/abseil/abseil-cpp.git third_party/abseil-cpp
    ```

    Note: Abseil is required by the examples and tests. libgav1 will depend on
    it if `LIBGAV1_THREADPOOL_USE_STD_MUTEX` is set to `0` (see below).

4.  (Optional) [GoogleTest](https://github.com/google/googletest)

    From within the libgav1 directory:

    ```shell
    $ git clone -b release-1.12.1 --depth 1 \
      https://github.com/google/googletest.git third_party/googletest
    ```

### Compile

```shell
  $ mkdir build && cd build
  $ cmake -G "Unix Makefiles" ..
  $ make
```

Configuration options:

*   `LIBGAV1_MAX_BITDEPTH`: defines the maximum supported bitdepth (8, 10, 12;
    default: 12).
*   `LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS`: define to a non-zero value to disable
    [symbol reduction](#symbol-reduction) in an optimized build to keep all
    versions of dsp functions available. Automatically defined in
    `src/dsp/dsp.h` if unset.
*   `LIBGAV1_ENABLE_AVX2`: define to a non-zero value to enable avx2
    optimizations. Automatically defined in `src/utils/cpu.h` if unset.
*   `LIBGAV1_ENABLE_NEON`: define to a non-zero value to enable NEON
    optimizations. Automatically defined in `src/utils/cpu.h` if unset.
*   `LIBGAV1_ENABLE_SSE4_1`: define to a non-zero value to enable sse4.1
    optimizations. Automatically defined in `src/utils/cpu.h` if unset. Note
    setting this to 0 will also disable AVX2.
*   `LIBGAV1_ENABLE_LOGGING`: define to 0/1 to control debug logging.
    Automatically defined in `src/utils/logging.h` if unset.
*   `LIBGAV1_EXAMPLES_ENABLE_LOGGING`: define to 0/1 to control error logging in
    the examples. Automatically defined in `examples/logging.h` if unset.
*   `LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK`: define to 1 to enable transform
    coefficient range checks.
*   `LIBGAV1_LOG_LEVEL`: controls the maximum allowed log level, see `enum
    LogSeverity` in `src/utils/logging.h`. Automatically defined in
    `src/utils/logging.cc` if unset.
*   `LIBGAV1_THREADPOOL_USE_STD_MUTEX`: controls use of std::mutex and
    absl::Mutex in ThreadPool. Defining this to 1 will remove any Abseil
    dependency from the core library. Automatically defined in
    `src/utils/threadpool.h` if unset. Defaults to 1 on Android & iOS, 0
    otherwise.
*   `LIBGAV1_MAX_THREADS`: sets the number of threads that the library is
    allowed to create. Has to be an integer > 0. Otherwise this is ignored. The
    default value is 128.
*   `LIBGAV1_FRAME_PARALLEL_THRESHOLD_MULTIPLIER`: the threshold multiplier that
    is used to determine when to use frame parallel decoding. Frame parallel
    decoding will be used if |threads| > |tile_count| * this multiplier. Has to
    be an integer > 0. The default value is 4. This is an advanced setting
    intended for testing purposes.

For additional options see:

```shell
  $ cmake .. -LH
```

## Testing

*   `gav1_decode` can be used to decode IVF files, see `gav1_decode --help` for
    options. Note: tools like [FFmpeg](https://ffmpeg.org) can be used to
    convert other container formats to IVF.

*   Unit tests are built when `LIBGAV1_ENABLE_TESTS` is set to `1`. The binaries
    can be invoked directly or with
    [`ctest`](https://cmake.org/cmake/help/latest/manual/ctest.1.html).

    *   The test input location can be given by setting the
        `LIBGAV1_TEST_DATA_PATH` environment variable; it defaults to
        `<libgav1_src>/tests/data`, where `<libgav1_src>` is `/data/local/tmp`
        on Android platforms or the source directory configured with cmake
        otherwise.

    *   Output is written to the value of the `TMPDIR` or `TEMP` environment
        variables in that order if set, otherwise `/data/local/tmp` on Android
        platforms, the value of `LIBGAV1_FLAGS_TMPDIR` if defined during
        compilation or the current directory if not.

## Development

### Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to submit patches.

### Style

libgav1 follows the
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html) with
formatting enforced by `clang-format`.

### Comments

Comments of the form '`// X.Y(.Z).`', '`Section X.Y(.Z).`' or '`... in the
spec`' reference the relevant section(s) in the
[AV1 specification](http://aomediacodec.github.io/av1-spec/av1-spec.pdf).

### DSP structure

*   `src/dsp/dsp.cc` defines the main entry point: `libgav1::dsp::DspInit()`.
    This handles cpu-detection and initializing each logical unit which populate
    `libgav1::dsp::Dsp` function tables.
*   `src/dsp/dsp.h` contains function and type definitions for all logical units
    (e.g., intra-predictors)
*   `src/utils/cpu.h` contains definitions for cpu-detection
*   base implementations are located in `src/dsp/*.{h,cc}` with platform
    specific optimizations in sub-folders
*   unit tests define `DISABLED_Speed` test(s) to allow timing of individual
    functions

#### Symbol reduction

Based on the build configuration unneeded lesser optimizations are removed using
a hierarchical include and define system. Each logical unit in `src/dsp` should
include all platform specific headers in descending order to allow higher level
optimizations to disable lower level ones. See `src/dsp/loop_filter.h` for an
example.

Each function receives a new define which can be checked in platform specific
headers. The format is: `LIBGAV1_<Dsp-table>_FunctionName` or
`LIBGAV1_<Dsp-table>_[sub-table-index1][...-indexN]`, e.g.,
`LIBGAV1_Dsp8bpp_AverageBlend`,
`LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDc`. The Dsp-table name is of
the form `Dsp<bitdepth>bpp` e.g. `Dsp10bpp` for bitdepth == 10 (bpp stands for
bits per pixel). The indices correspond to enum values used as lookups with
leading 'k' removed. Platform specific headers then should first check if the
symbol is defined and if not set the value to the corresponding
`LIBGAV1_CPU_<arch>` value from `src/utils/cpu.h`.

```
  #ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDc
  #define LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDc LIBGAV1_CPU_SSE4_1
  #endif
```

Within each module the code should check if the symbol is defined to its
specific architecture or forced via `LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS` before
defining the function. The `DSP_ENABLED_(8|10)BPP_*` macros are available to
simplify this check for optimized code.

```
  #if DSP_ENABLED_8BPP_SSE4_1(TransformSize4x4_IntraPredictorDc)
  ...

  // In unoptimized code use the following structure; there's no equivalent
  // define for LIBGAV1_CPU_C as it would require duplicating the function
  // defines used in optimized code for only a small benefit to this
  // boilerplate.
  #if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  ...
  #else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  #ifndef LIBGAV1_Dsp8bpp_TransformSize4x4_IntraPredictorDcFill
  ...
```

## Bugs

Please report all bugs to the issue tracker:
https://issuetracker.google.com/issues/new?component=750480&template=1355007

## Discussion

Email: gav1-devel@googlegroups.com

Web: https://groups.google.com/forum/#!forum/gav1-devel
