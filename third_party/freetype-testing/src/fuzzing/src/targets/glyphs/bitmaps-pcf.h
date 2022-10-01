// bitmaps-pcf.h
//
//   Fuzz target for PCF glyph bitmaps.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_GLYPHS_BITMAPS_PCF_H_
#define TARGETS_GLYPHS_BITMAPS_PCF_H_


#include "targets/glyphs/bitmaps.h"


namespace freetype {


  class GlyphsBitmapsPcfFuzzTarget
    : public GlyphsBitmapsFuzzTarget
  {
  public:


    GlyphsBitmapsPcfFuzzTarget();
  };
}


#endif // TARGETS_GLYPHS_BITMAPS_PCF_H_
