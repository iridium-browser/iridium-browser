// facevisitor-subglyphs.h
//
//   Try to find glyphs that can be split into subglyphs.
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


#ifndef VISITORS_FACE_VISITOR_SUBGLYPHS_H_
#define VISITORS_FACE_VISITOR_SUBGLYPHS_H_


#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorSubGlyphs
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    static const FT_Long   GLYPH_INDEX_MAX    = 30;
    static const FT_Long   SUBGLYPH_INDEX_MAX = 10;
    static const FT_Int32  LOAD_FLAGS         = FT_LOAD_NO_BITMAP |
                                                FT_LOAD_NO_RECURSE;
  };
}


#endif // VISITORS_FACE_VISITOR_SUBGLYPHS_H_
