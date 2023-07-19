// glyphloaditerator.h
//
//   Base class of iterators over glyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_GLYPH_LOAD_ITERATOR_H_
#define ITERATORS_GLYPH_LOAD_ITERATOR_H_

#include <memory> // std::unique_ptr
#include <vector>

#include "iterators/glyphrenderiterator.h"
#include "utils/noncopyable.h"
#include "utils/utils.h"
#include "visitors/glyphvisitor.h"


namespace freetype {


  class GlyphLoadIterator
    : private noncopyable
  {
  public:


    // @See: `GlyphLoadIterator::set_num_load_glyphs()'.

    GlyphLoadIterator( FT_Long  num_load_glyphs );


    virtual
    ~GlyphLoadIterator() = default;


    virtual void
    run( Unique_FT_Face  face ) = 0;


    // @Description:
    //   Define an upper bound of how many glyphs should be loaded.  Note
    //   that some iterators require this function to be called BEFORE
    //   `GlyphLoadIterator::run()'.  This function can be invoked by using
    //   the dedicated constructor.
    // @Input
    //   num_load_glyphs ::
    //     The amount of glyphs that should be loaded (if available).

    void
    set_num_load_glyphs( FT_Long  num_load_glyphs );


    void
    add_load_flags( FT_Int32  flags );


    // @Description:
    //   Add a glpyh visitor that is called with every loaded glyph.
    //
    // @Input:
    //   visitor ::
    //     A glyph visitor.
    //
    // @Return:
    //   A reference to the added visitor.

    void
    add_visitor( std::unique_ptr<GlyphVisitor>  visitor );


    // @Description:
    //   Add a glyph render iterator that is called with every loaded glyph.
    //
    // @Input:
    //   iterator ::
    //     A glyph render iterator.
    //
    // @Return:
    //   A reference to the added iterator.

    void
    add_iterator( std::unique_ptr<GlyphRenderIterator>  iterator );

    
  protected:


    FT_Long   num_load_glyphs = -1;
    FT_Int32  load_flags      = FT_LOAD_DEFAULT;

    std::vector<std::unique_ptr<GlyphVisitor>>         glyph_visitors;
    std::vector<std::unique_ptr<GlyphRenderIterator>>  glyph_render_iterators;


    // @Description:
    //   Invoke all visitors and iterators with a copy of the glyph.
    //
    // @Input:
    //   glyph ::
    //     Basic glyph that will be copied for every visitor and every
    //     iterator.

    void
    invoke_visitors_and_iterators( const Unique_FT_Glyph&  glyph );
  };
}


#endif // ITERATORS_GLYPH_LOAD_ITERATOR_H_
