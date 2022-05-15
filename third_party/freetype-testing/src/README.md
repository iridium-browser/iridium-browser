[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org/

FreeType is a freely available software library to render fonts.

# Testing

This repository provides testing utilities for FreeType:

- [**Fuzzing**](https://github.com/freetype/freetype2-testing/tree/master/fuzzing): house the fuzz targets for [OSS-Fuzz](https://github.com/google/oss-fuzz/) and use [Travis CI](https://travis-ci.com/freetype/freetype2-testing) to run a regression test suite of fuzzed samples that uncovered verified and fixed bugs.
