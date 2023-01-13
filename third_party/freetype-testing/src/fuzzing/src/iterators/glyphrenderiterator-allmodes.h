// glyphrenderiterator-allmodes.h
//
//   Iterator that renders glyphs with all available modes:
//     - normal
//     - light
//     - mono
//     - lcd
//     - lcd v
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_GLYPH_RENDER_ITERATOR_ALL_MODES_H_
#define ITERATORS_GLYPH_RENDER_ITERATOR_ALL_MODES_H_


#include "iterators/glyphloaditerator.h"


namespace freetype {


  class GlyphRenderIteratorAllModes
    : public GlyphRenderIterator
  {
  public:


    void
    run( Unique_FT_Glyph  glyph )
    override;
  };
}

#endif // ITERATORS_GLYPH_RENDER_ITERATOR_ALL_MODES_H_
