---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: running-unit-tests-on-the-target
title: Running Unit Tests On The Target
---

In an ideal world, you'd mock out all of your dependencies so you can run your
unit tests on your development machine.

When this isn't feasible, don't let it stand in your way:

Add a USE flag "install-tests" to your ebuild:

```none
IUSE="install_tests"
```

and conditionalize your unit test libraries on this flag:

```none
RDEPEND="blah
    blah
	install_tests? ( dev-cpp/gtest )"
```

Then build the use flag into a make flag:

```none
use install_tests && MAKE_FLAGS="INSTALL_TESTS=1"
```

and pass that to your package:

```none
src_compile() {
	tc-export CXX PKG_CONFIG
	cros-debug-add-NDEBUG
	emake ${MAKE_FLAGS} PLUGINDIR="${PLUGINDIR}" || die "Failed to compile"
}
src_install() {
	emake ${MAKE_FLAGS} DESTDIR=${D} install || die "Install failed"
}
```

see cromo's ebuild for an example

To develop with gmerge, set this flag on the commandline when you start your
devserver:

```none
USE=install_tests ./start_devserver 
```

Note that you must do this within the chroot. If you run the devserver from
outside of the chroot, it will drop its environment in the process of entering
the chroot, and your USE flag will get dropped.
