This directory contains patch files for `rust-openssl` for it to build successfully with
`--features=unstable_boringssl`.

After running `prepare-rust-openssl`, the `rust-openssl` git repo is cloned to
`beto-rust/boringssl-build/rust-openssl/openssl`, and the patches in this directory will be applied.

If you make further changes, or update the "base commit" in `prepare-rust-openssl`, you can
regenerate the patch files by following these steps:

1. Run `cargo run -- prepare-rust-openssl`
2. `cd boringssl-build/rust-openssl/` and make the necessary changes
3. Commit the changes
4. `git format-patch BASE_COMMIT`. (Note: `BASE_COMMIT` is set by `prepare-rust-openssl`)
5. The patch files will be generated in the current working directory. Move them here in
   `nearby/scripts/openssl-patches`.

### Regenerate patches based on AOSP changes

In the "make the necessary changes" part in Step 2 above, follow these steps:

1. Download the patch files in https://googleplex-android.googlesource.com/platform/external/rust/crates/openssl/+/master/patches
2. `cd` into the openssl directory since the AOSP project starts at that root:
   ```sh
   $ cd openssl
   ```
3. Reset your branch to `BASE_COMMIT` to ensure the AOSP patches apply cleanly.
   ```sh
   $ git co BASE_COMMIT
   $ git co -b create-patch
   ```
4. Apply the patches from AOSP
   ```sh
   for i in /path/to/android/external/rust/crates/openssl/patches/*; do patch -p1 < $i; done
   ```
5. Remove unneeded files (like `.orig`). Commit the changes.
6. Patches locally in `scripts/openssl-patches` but not in AOSP are lost in this process. Reapply
   the appropriate ones at this point, using `git apply` or `git am`.
7. Continue with `git format-patch` described in step 4 in the previous section.
