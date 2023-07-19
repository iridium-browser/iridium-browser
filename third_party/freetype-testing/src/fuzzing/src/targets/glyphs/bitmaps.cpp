// bitmaps.cpp
//
//   Implementation of GlyphsBitmapsFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/glyphs/bitmaps.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "iterators/glyphloaditerator-naive.h"
#include "visitors/glyphvisitor-bitmap-handling.h"
#include "utils/logging.h"


  const FT_Long
  freetype::GlyphsBitmapsFuzzTarget::
  NUM_LOAD_GLYPHS = 20;


  freetype::GlyphsBitmapsFuzzTarget::
  GlyphsBitmapsFuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();
    auto  fpi = freetype::make_unique<FacePrepIteratorBitmaps>();
    auto  gli =
      freetype::make_unique<GlyphLoadIteratorNaive>( NUM_LOAD_GLYPHS );


    // -----------------------------------------------------------------------
    // Glyph load iterators:

    (void) gli
      ->add_visitor( freetype::make_unique<GlyphVisitorBitmapHandling>() );

    // -----------------------------------------------------------------------
    // Assemble the rest:

    (void) fpi->add_iterator( std::move( gli ) );

    (void) fli->add_iterator( std::move( fpi ) );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_iterator( std::move( fli ) );
  }
