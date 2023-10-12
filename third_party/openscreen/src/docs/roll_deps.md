# Updating Dependencies

The Open Screen Library uses several external libraries that are compiled from
source and fetched from additional git repositories into folders below
`third_party/`.  These libraries are listed in the `DEPS` file in the root of
the repository.  (`DEPS` listed with `dep_type: cipd` are not managed through git.)

Periodically the versions of these libraries are updated to pick up bug and
security fixes from upstream repositories. 

Note that depot_tools is in the process of migrating dependency management from
`DEPS` to git submodules.

The process is roughly as follows:

1. Identify the git commit for the new version of the library that you want to roll.
   * For libraries that are also used by Chromium, prefer rolling to the same version
     listed in [`Chromium DEPS`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/DEPS).
     These libraries are listed below.
   * For other libraries, prefer the most recent stable or general availability release.
2. Use the `roll-dep` script to update the git hashes in `DEPS` and the corresponding git submodule.
   * This will generate and upload a CL to Gerrit.
   * Example: `roll-dep -r jophba --roll-to 229b04537a191ce272a96734b0f4ddcccc31f241 third_party/quiche/src`
   * More documentation in [Rolling Dependencies](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/dependencies.md#rolling-dependencies)
3. Update your CL if needed, such as updating the `BUILD.gn` file for the
   corresponding library to handle files moving around, compiler flag
   adjustments, etc.
4. Use a tryjob to verify that nothing is broken and send for code review.

## Libraries also used by Chromium

* `third_party/libprotobuf-mutator/src`
* `third_party/jsoncpp/src`
* `third_party/googletest/src` (not kept in sync with Chromium, see comment in `DEPS`)
* `third_party/quiche/src`
* `third_party/abseil/src` (not kept in sync with Chromium, see comment in `DEPS`)
* `third_party/libfuzzer/src`
* `third_party/googleurl/src` (not kept in sync with Chromium, see comment in `DEPS`)

## Special cases that are updated manually

* `third_party/protobuf`: See instructions in `third-party/protobuf/README.chromium`
* `third_party/boringssl/src`: See instructions in `third_party/boringssl/README.openscreen.md`
* `third_party/mozilla`: See README.md



