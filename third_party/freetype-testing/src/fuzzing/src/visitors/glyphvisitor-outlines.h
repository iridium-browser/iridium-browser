// glyphvisitor-outlines.h
//
//   Invoke API functions that deal with outline processing and are not used
//   by FreeType itself.
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


#ifndef VISITORS_GLYPH_VISITOR_OUTLINES_H_
#define VISITORS_GLYPH_VISITOR_OUTLINES_H_


#include <vector>

#include "utils/utils.h"
#include "visitors/glyphvisitor.h"


namespace freetype {


  class GlyphVisitorOutlines
    : public GlyphVisitor
  {
  public:


    GlyphVisitorOutlines();


    void
    run( Unique_FT_Glyph  glyph )
    override;


  private:


    typedef void  (*OutlineFunc)( FT_Outline& );

    std::vector<OutlineFunc>  callbacks;


    // @Description:
    //   `FT_Outline_Reverse( outline )'.

    static void 
    reverse( FT_Outline&  outline );


    // @Description:
    //   `FT_Outline_Embolden( outline, strength )'.

    static void
    embolden( FT_Outline&  outline,
              FT_Pos       strength );


    // @Description:
    //   `FT_Outline_EmboldenXY( outline, xstrength, ystrength )'.

    static void
    embolden( FT_Outline&  outline,
              FT_Pos       xstrength,
              FT_Pos       ystrength );


    // @Description:
    //   `GlyphVisitorOutlines::embolden( outline, ... )'.

    static void
    embolden_1( FT_Outline&  outline );


    // @Description:
    //   `GlyphVisitorOutlines::embolden( outline, ... )'.

    static void
    embolden_2( FT_Outline&  outline );


    // @Description:
    //   `GlyphVisitorOutlines::embolden( outline, ... )'.

    static void
    embolden_3( FT_Outline&  outline );


    // @Description:
    //   `GlyphVisitorOutlines::embolden( outline, ... )'.

    static void
    embolden_4( FT_Outline&  outline );
  };
}


#endif // VISITORS_GLYPH_VISITOR_OUTLINES_H_
