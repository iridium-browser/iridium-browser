// facevisitor-loadglyphs-outlines.h
//
//   Load outline glyphs with a variety of different load flags.
//
//   Drivers:
//     - CFF
//     - CID Type 1
//     - PFR
//     - TrueType
//     - Type 1
//     - Type 42
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_LOAD_GLYPHS_OUTLINES_H_
#define VISITORS_FACE_VISITOR_LOAD_GLYPHS_OUTLINES_H_


#include "visitors/facevisitor-loadglyphs.h"


namespace freetype {


  class FaceVisitorLoadGlyphsOutlines
    : public FaceVisitorLoadGlyphs
  {
  public:


    FaceVisitorLoadGlyphsOutlines();


  private:


    static const FT_Long  NUM_USED_GLYPHS;

    FT_Matrix  matrix;
    FT_Vector  delta;
  };
}


#endif // VISITORS_FACE_VISITOR_LOAD_GLYPHS_OUTLINES_H_
