[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# Legacy Target

This folder contains FreeType's legacy target for OSS-Fuzz.  Development on
the legacy target stopped in Summer 2018 when FreeType moved on to using more
dedicated targets that are hosted in this repository.  Nevertheless, the
legacy target will continue to be fuzzed as its slightly different approach
could uncover flaws that the other targets miss.

Before this folder was moved to
[freetype-testing](https://github.com/freetype/freetype2-testing), it lived in
[freetype2/src/tools/ftfuzzer](http://git.savannah.gnu.org/cgit/freetype/freetype2.git)
where the history of the contained files can be found.

For more details on the files in this folder see
[README.old](https://github.com/freetype/freetype2-testing/blob/master/fuzzing/src/legacy/README.old).
