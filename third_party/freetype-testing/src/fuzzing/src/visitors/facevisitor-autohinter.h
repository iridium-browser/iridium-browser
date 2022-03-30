// facevisitor-autohinter.h
//
//   Load glyphs with the autohinter.
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


#ifndef VISITORS_FACE_VISITOR_AUTOHINTER_H_
#define VISITORS_FACE_VISITOR_AUTOHINTER_H_


#include <string>
#include <vector>

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorAutohinter
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    static const FT_Long   GLYPH_INDEX_MAX = 30;
    static const FT_Int32  LOAD_FLAGS      = FT_LOAD_FORCE_AUTOHINT |
                                             FT_LOAD_NO_BITMAP;

    FT_Bool               default_warping = 0;
    std::vector<FT_Bool>  warpings{ 0, 1 };


    void
    set_property( Unique_FT_Face&    face,
                  const std::string  property_name,
                  const void*        value );

    void
    load_glyphs( Unique_FT_Face&  face );
  };
}


#endif // VISITORS_FACE_VISITOR_AUTOHINTER_H_
