// bitmaps.h
//
//   Fuzz target for glyph bitmaps.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_GLYPHS_BITMAPS_H_
#define TARGETS_GLYPHS_BITMAPS_H_


#include "targets/FaceFuzzTarget.h"


namespace freetype {


  class GlyphsBitmapsFuzzTarget
    : public FaceFuzzTarget
  {
  public:


    GlyphsBitmapsFuzzTarget();


  private:


    static const FT_Long  NUM_LOAD_GLYPHS;
  };
}


#endif // TARGETS_GLYPHS_BITMAPS_H_
