---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: undefinedbehaviorsanitizer
title: UndefinedBehaviorSanitizer (UBSan)
---

UndefinedBehaviorSanitizer (UBSan) is a fast undefined behavior detector
implemented in Clang and Compiler-rt. Various computations will be instrumented
to detect undefined behavior at runtime.

For now, only 64-bit Linux platform is tested. Various compile flags to use
UBSan is available at
<http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation>.

### Building Chromium with UBSan

UBSan builds are experimentally supported by Chromium, and can be built as
below. is_ubsan=true automatically enforces to use Clang as a build compiler.
Please note that is_ubsan=true excludes -fsanitize=vptr, which is also part of
the undefined behavior sanitizer.

```none
gn args out/ubsan
# set is_ubsan = true
# set is_debug = false
ninja -C out/ubsan chrome
```

To use -fsanitize=vptr, the is_ubsan_vptr options can be used. is_ubsan_vptr
loads the blocklist from src/tools/ubsan_vptr/ignorelist.txt.

```none
gn args out/ubsan
# set is_ubsan_vptr = true
ninja -C out/ubsan chrome
```

Pre-built Chrome binaries are available at
http://commondatastorage.googleapis.com/chromium-browser-ubsan/index.html?prefix=linux-release-vptr/

### Runtime Flags

UBSan also supports common runtime flags with UBSAN_OPTIONS like other
sanitizers. Followings are UBSan specific runtime flags.

*   print_stacktrace : print the stacktrace when UBSan reports an error.
*   suppressions : suppress an error report at runtime.
