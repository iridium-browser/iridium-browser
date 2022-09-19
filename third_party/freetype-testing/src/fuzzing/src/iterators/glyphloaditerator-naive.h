// glyphloaditerator-naive.h
//
//   Iterator that, naively, loads every available glyph.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_GLYPH_LOAD_ITERATOR_NAIVE_H_
#define ITERATORS_GLYPH_LOAD_ITERATOR_NAIVE_H_


#include "iterators/glyphloaditerator.h"


namespace freetype {


  class GlyphLoadIteratorNaive
    : public GlyphLoadIterator
  {
  public:


    GlyphLoadIteratorNaive( FT_Long  num_load_glyphs )
      : GlyphLoadIterator( num_load_glyphs ) {}


    void
    run( Unique_FT_Face  face )
    override;

  };
}


#endif // ITERATORS_GLYPH_LOAD_ITERATOR_NAIVE_H_
