// facevisitor-renderglyphs.h
//
//   Render a bunch of glyphs with a variety of different modes.
//
//   Drivers:
//     - CID Type 1
//     - CFF
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


#ifndef VISITORS_FACE_VISITOR_RENDER_GLYPHS_H_
#define VISITORS_FACE_VISITOR_RENDER_GLYPHS_H_


#include <vector>
#include <utility> // std::pair

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorRenderGlyphs
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    typedef std::vector<std::pair<FT_Int32, FT_Render_Mode>>  RenderModes;

    static const FT_Long      GLYPH_INDEX_MAX = 5;
    static const RenderModes  RENDER_MODES;
  };
}


#endif // VISITORS_FACE_VISITOR_RENDER_GLYPHS_H_
