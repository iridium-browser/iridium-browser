// glyphvisitor.h
//
//   Base class of visitors of glyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_GLYPH_VISITOR_H_
#define VISITORS_GLYPH_VISITOR_H_


#include "utils/noncopyable.h"
#include "utils/utils.h"


namespace freetype {


  class GlyphVisitor
    : private noncopyable
  {
  public:


    virtual
    ~GlyphVisitor() = default;


    // @Description:
    //   Run an arbitrary action on a glyph.
    //
    // @Input:
    //   glyph ::
    //     No restrictions apply -- use it at will.

    virtual void
    run( Unique_FT_Glyph  glyph ) = 0;
  };
}


#endif // VISITORS_GLYPH_VISITOR_H_
