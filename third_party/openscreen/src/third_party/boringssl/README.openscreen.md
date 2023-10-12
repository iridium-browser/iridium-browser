# Updating boringssl

1. Roll the dependency following the [instructions](../../docs/roll_deps.md).
2. Run `gclient sync` to sync local copy of boringssl with upstream.
3. Go to `boringssl/src` and run `util/generate_build_files.py` to update the
   generated GN files.
4. Attempt to build locally.  Often adjustments to our `BUILD.gn` must also be
   made.  Look at the Chromium copy of the [boringssl BUILD.gn](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/third_party/boringssl/BUILD.gn)
   for hints.
5. Attempt a tryjob.
