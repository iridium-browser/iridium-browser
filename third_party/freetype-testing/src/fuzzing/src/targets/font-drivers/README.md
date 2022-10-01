[![License: GPL
v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# Font Driver Targets

All font driver targets use a similar execution scheme:

1. Have a [`FaceLoadIterator`](/fuzzing/src/iterators/faceloaditerator.h) load
   faces (and variations) of font files.
    - Attach interesting [`FaceVisitor`](/fuzzing/src/visitors/facevisitor.h).
2. Have a [`FacePrepIterator`](/fuzzing/src/iterators/faceprepiterator.h)
   prepare all loaded faces for usage with either bitmaps or outlines.
    - Attach interesting [`FaceVisitor`](/fuzzing/src/visitors/facevisitor.h).
