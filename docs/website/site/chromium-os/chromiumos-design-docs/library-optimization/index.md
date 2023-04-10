---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: library-optimization
title: Library Optimization
---

## Problem

One of the optimizations proposed for Chrome OS was the shrinking of C libraries
by removing the unused symbols from them.
This document describes the work done in this direction.

## Existing solution

Prior work has been done in this field by Montavista, which released an open
source set of scripts called the [Library Optimizer
Tool](http://libraryopt.sourceforge.net/) (libopt). According to their site:

The Library Optimizer examines the complete target file system, resolves all
shared library symbol references, and rebuilds the shared libraries with only
the object files required to satisfy the symbol references

Sadly the script is not very well documented (both in the way someone could use
it without modifying it and the way it works internally). After examining the
contents of the script and the example build script for libc it seems that the
script does indeed return a list of .o files which should be included in the
library after analyzing symbol dependencies. It can also receive a strip
parameter which uses the strip command to strip the .notes and the .comments
sections from the library.
I have modified the script to print out a lot of debug information and I have
created my own simple library to test the script on. Sadly the script did not
work as expected, as it didn’t output the expected .o file names.

## New solution

Some of the members of the compiler team came up with a new solution, which
would still involve passing a list of symbols to the linker, but which is less
error-prone as the linker can automatically detect the dependencies which these
symbols introduce.
The binaries can apparently be stripped of unused symbols using the garbage
collector, however this is not done by default yet on all Chromium OS packages.
Stripping the libraries is the interesting part, but this cannot be done
automatically without a view of the whole system. The solution presented by the
compiler team involves manipulating the visibility of functions and using
function sections and garbage collection to eliminate the unused symbols from
the library.
The current problems with this approach are:

1.  Using -ffunction-sections is supposed to increase the library size
            but is needed for the shrink, must investigate if how much space is
            saved
2.  dlopen()/dlsym() breaks this method so they will have to be allowed
            somehow

## Current work

A script has been created which currently:

1.  Builds a list of all the undefined symbols used by executables.
2.  See what symbols are not used anywhere and marks them as unused

The next steps are:

1.  Relink the libraries so that the unused symbols get removed (start
            off with a small number of libraries)
2.  Make it work with dlopen/dlsym.
