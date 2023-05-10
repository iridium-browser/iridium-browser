// facevisitor-loadglyphs-bitmaps.h
//
//   Load bitmap glyphs with a variety of different load flags.
//
//   Drivers: all
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_LOAD_GLYPHS_BITMAPS_H_
#define VISITORS_FACE_VISITOR_LOAD_GLYPHS_BITMAPS_H_


#include "visitors/facevisitor-loadglyphs.h"


namespace freetype {


  class FaceVisitorLoadGlyphsBitmaps
    : public FaceVisitorLoadGlyphs
  {
  public:


    FaceVisitorLoadGlyphsBitmaps();


  private:


    static const FT_Long  NUM_USED_GLYPHS;
  };
}


#endif // VISITORS_FACE_VISITOR_LOAD_GLYPHS_BITMAPS_H_
