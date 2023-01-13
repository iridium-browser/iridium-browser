[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# How To Source

The fuzzing subproject can build a large number of different executables and,
thus, understanding the structure of the source code is a key element in
understanding what can be built in general.

Please check the `README.md` files in the corresponding folders for detailed
information.

## Executables

- [**driver**](driver):   the driver executable handles any kind of input
                          file.
- [**fuzzers**](fuzzers): this is where the fuzzer executables are assembled.

## Targets

- [**legacy**](legacy):   the home of the old target (half-retired in 2018).
- [**targets**](targets): blueprints for all new fuzz targets.  This folder
                          "exports" a library that contains all targets and
                          can be used conveniently by the [driver](driver) and
                          the [fuzzers](fuzzers).

## Utilities

- [**iterators**](iterators):  various iterators over faces and glyphs.
- [**utils**](utils):          utilities and tools that can be used 
                               everywhere.
- [**visitors**](visitors):    dedicated visitors that can be attached to
                               iterators.
