// glyphvisitor-transform.h
//
//   Visitor that transform a given glyph.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_GLYPH_VISITOR_TRANSFORM_H_
#define VISITORS_GLYPH_VISITOR_TRANSFORM_H_


#include "visitors/glyphvisitor.h"


namespace freetype {


  class GlyphVisitorTransform
    : public GlyphVisitor
  {
  public:


    void
    run( Unique_FT_Glyph  glyph )
    override;


  private:


    void
    transform( const Unique_FT_Glyph&  glyph,
               FT_Matrix*              matrix,
               FT_Vector*              delta );
  };
}


#endif // VISITORS_GLYPH_VISITOR_TRANSFORM_H_
