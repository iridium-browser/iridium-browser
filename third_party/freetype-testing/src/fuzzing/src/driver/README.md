[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# The Driver

The driver is supposed to handle all kinds of input files (for which
[targets](/fuzzing/src/targets) exist).

## Overview

The current version of the driver uses a switch to decide between the
different targets:

```
$ ./driver --foo /path/to/foo/file
```

Please refer to the usage message of the driver for a full list of all
available targets:

```
$ ./driver
```

## Build Options

The build setup process of the driver listens to an environment variable:

- `CMAKE_DRIVER_EXE_NAME`:  set a name for the executable (default: `driver`).
