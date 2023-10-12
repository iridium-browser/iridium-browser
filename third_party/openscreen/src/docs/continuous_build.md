# Continuous build and try jobs

Open Screen uses [LUCI builders](https://ci.chromium.org/p/openscreen/builders)
to monitor the build and test health of the library.

Current builders include:

| Name                   | Arch   | OS                     | Toolchain | Build   | Notes                  |
|------------------------|--------|------------------------|-----------|---------|------------------------|
| linux64_debug          | x86-64 | Ubuntu Linux 20.04     | clang     | debug   | ASAN enabled           |
| linux_arm64_debug      | arm64  | Ubuntu Linux 20.04 [*] | clang     | debug   |                        |
| linux64_gcc_debug      | x86-64 | Ubuntu Linux 20.04     | gcc-9     | debug   |                        |
| linux64_tsan           | x86-64 | Ubuntu Linux 20.04     | clang     | release | TSAN enabled           |
| linux64_coverage_debug | x86-64 | Ubuntu Linux 20.04     | clang     | debug   | used for code coverage |
| linux_arm64_cast_debug | arm64  | Ubuntu Linux 20.04 [*] | clang     | debug   | Builds cast standalone |
| mac_debug              | x86-64 | Mac OS X/Xcode         | clang     | debug   |                        |
| chromium_linux64_debug | x86-64 | Ubuntu Linux 20.04     | clang     | debug   | built with chromium    |
| chromium_mac_debug     | x86-64 | Mac OS X 10.15         | clang     | debug   | built with chromium    |
<br />

[*] Tests run on Ubuntu 20.04, but are cross-compiled to arm64 with a debian
sysroot.

The chromium_ builders compile against Chromium top-of-tree to ensure that
changes can be autorolled into Chromium.

You can run a patch through all builders using `git cl try` or the Gerrit Web
interface.  All builders are run as part of the commit queue and are also run
continuously in our CI.
