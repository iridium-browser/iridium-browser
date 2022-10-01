---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/deep-memory-profiler
  - Deep Memory Profiler
page_name: cpp-object-type-identifier
title: C++ Object Type Identifier (a.k.a. Type Profiler)
---

## Type Profiler no longer exists, see [crbug.com/490464](https://code.google.com/p/chromium/issues/detail?id=490464).

## Introduction

The C++ Object Type Identifier (a.k.a. Type Profiler) is another profiling
feature to find “Which type is the object?". For example, it reports:

> "An object at 0x37f3c88 is an instance of std::string."

RTTI is not enough to find a type of an object only from its address \[1\]. You
can try it with the following steps. See the [Design
Doc](https://docs.google.com/document/d/1DvgxYxrMH_v196YhPebAMaD7qMR3utTN4-ytVdZLPWE/edit)
if interested.

---

## How to Build with Type Profiler

1.  Edit .gclient to checkout the modified version of Clang.

    > `solutions = [`

    > ` { "name" : "src",`

    > ` "url" : "http://git.chromium.org/chromium/src.git",`

    > ` ...`

    > ` "custom_deps" : {`

    > ` ...`

    > ` "src/third_party/llvm-allocated-type":
    > "http://src.chromium.org/chrome/trunk/deps/third_party/llvm-allocated-type",`

    > ` ...`

    > ` },`

    > ` ...`

    > ` },`

    > `]`

2.  Build Chromium with additional build options.
            ("clang_type_profiler=1" is the required option.)

    > `export GYP_DEFINES='clang_type_profiler=1'`

    > `export GYP_GENERATORS='ninja'`

    > `export GYP_GENERATOR_FLAGS='output_dir=out_type_profile'`

    > `gclient runhooks`

    > `ninja -C out_type_profile/Debug -j 16 chrome`

---

## How to Use

## In a part of [Deep Memory Profiler](/developers/deep-memory-profiler)

1.  Run the customized Chromium with [Deep Memory
            Profiler](/developers/deep-memory-profiler) as usual.
2.  Run the dmprof script with a policy label "t0".

    > `tools/deep_memory_profiler/dmprof csv -p **t0**
    > ~/profile/00-test.12345.0002.heap > ~/profile/00-test.12345.result.csv`

## Standalone

1.  Run the customized Chromium with TCMalloc's heap-profiler and an
            environment variable "HEAP_PROFILE_TYPE_STATISTICS=1".

    > `HEAPPROFILE=/tmp/prefix HEAP_PROFILE_TIME_INTERVAL=20
    > HEAP_PROFILE_TYPE_STATISTICS=1 out_type_profile/Debug/chrome --no-sandbox`

2.  Type statistics "prefix.&lt;pid&gt;.????.type" is dumped with every
            heap profile dump. It is a classification of all malloc'ed objects
            by their types. For example, " 13: 520 @ N3WTF5MutexE" means that 13
            objects of WTF::Mutex occupy 520 bytes.

    > type statistics:

    > 3739: 841574 @ (no_typeinfo)

    > 1: 16 @ N3WTF13WTFThreadDataE

    > 1: 16 @ N3WTF14ThreadSpecificINS_13WTFThreadDataEE4DataE

    > ...

    > 13: 520 @ N3WTF5MutexE

    > ...

## Looking up from your code

Type Profiler just records mapping from object addresses to their types. You can
utilize the mapping by yourself. To use it from your code,

1.  Include
            third_party/tcmalloc/chromium/src/gperftools/type_profiler_map.h,
            and
2.  Call LookupType() for object addresses.
    *   LookupType() returns const std::type_info& for the object.

---

## References

*   \[1\] Jozsef Mihalicza et al.: "[Type-preserving heap profiler for
            C++](http://ieeexplore.ieee.org/xpl/freeabs_all.jsp?arnumber=6080813)",
            Proceedings of the 2011 27th IEEE International Conference on
            Software Maintenance (ICSM '11).
