[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# The Fuzzers

In contrast to the [driver](/fuzzing/src/driver), the fuzzers cannot be built
as a monolith but every target has to be built as its own executable.  The
code of the fuzzers is the same code as the code of the
[driver](/fuzzing/src/driver) with the exception of it being split across
multiple files and invoked by `LLVMFuzzerTestOneInput` which uses fuzz targets
repeatedly (and not just once).
