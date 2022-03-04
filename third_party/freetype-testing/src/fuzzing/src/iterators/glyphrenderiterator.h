// glyphrenderiterator.h
//
//   Base class of iterators that render glyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_GLYPH_RENDER_ITERATOR_H_
#define ITERATORS_GLYPH_RENDER_ITERATOR_H_


#include <memory> // std::unique_ptr
#include <vector>

#include "utils/noncopyable.h"
#include "utils/utils.h"
#include "visitors/glyphvisitor.h"


namespace freetype {


  class GlyphRenderIterator
    : private noncopyable
  {
  public:


    virtual
    ~GlyphRenderIterator() = default;


    virtual void
    run( Unique_FT_Glyph  face ) = 0;


    // @Description:
    //   Add a glpyh visitor that is called with every rendered glyph.
    //
    // @Input:
    //   visitor ::
    //     A glyph visitor.

    void
    add_visitor( std::unique_ptr<GlyphVisitor>  visitor );

    
  protected:


    typedef std::vector<std::unique_ptr<GlyphVisitor>>  GlyphVisitors;


    GlyphVisitors  glyph_visitors;
  };
}


#endif // ITERATORS_GLYPH_RENDER_ITERATOR_H_
