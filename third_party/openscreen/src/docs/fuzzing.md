# Building and running fuzzers

In order to build fuzzers, you need the GN arg `use_libfuzzer=true`.  It's also
recommended to build with `is_asan=true` to catch additional problems.  Building
and running then might look like:
```bash
  gn gen out/libfuzzer --args="use_libfuzzer=true is_asan=true is_debug=false"
  ninja -C out/libfuzzer some_fuzz_target
  out/libfuzzer/some_fuzz_target <args> <corpus_dir> [additional corpus dirs]
```

The arguments to the fuzzer binary should be whatever is listed in the GN target
description (e.g. `-max_len=1500`).  These arguments may be automatically
scraped by Chromium's ClusterFuzz tool when it runs fuzzers, but they are not
built into the target.  You can also look at the file
`out/libfuzzer/some_fuzz_target.options` for what arguments should be used.  The
`corpus_dir` is listed as `seed_corpus` in the GN definition of the fuzzer
target.

