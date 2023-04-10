// outlines.cpp
//
//   Implementation of GlyphsOutlinesFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/glyphs/outlines.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-outlines.h"
#include "iterators/glyphloaditerator-naive.h"
#include "iterators/glyphrenderiterator-allmodes.h"
#include "visitors/glyphvisitor-cbox.h"
#include "visitors/glyphvisitor-outlines.h"
#include "visitors/glyphvisitor-transform.h"
#include "utils/logging.h"


  const FT_Long
  freetype::GlyphsOutlinesFuzzTarget::
  NUM_LOAD_GLYPHS = 5;


  freetype::GlyphsOutlinesFuzzTarget::
  GlyphsOutlinesFuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();
    auto  fpi = freetype::make_unique<FacePrepIteratorOutlines>();
    auto  gli =
      freetype::make_unique<GlyphLoadIteratorNaive>( NUM_LOAD_GLYPHS );
    auto  gri = freetype::make_unique<GlyphRenderIteratorAllModes>();


    // -----------------------------------------------------------------------
    // Glyph load iterators:

    (void) gli->add_load_flags( FT_LOAD_NO_BITMAP );

    (void) gli->add_iterator( std::move( gri ) );

    (void) gli->add_visitor( freetype::make_unique<GlyphVisitorCBox>()      );
    (void) gli->add_visitor( freetype::make_unique<GlyphVisitorOutlines>()  );
    (void) gli->add_visitor( freetype::make_unique<GlyphVisitorTransform>() );

    // -----------------------------------------------------------------------
    // Assemble the rest:

    (void) fpi->add_iterator( std::move( gli ) );

    (void) fli->set_supported_font_format( FaceLoader::FontFormat::TRUETYPE );
    (void) fli->add_iterator( std::move( fpi ) );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_iterator( std::move( fli ) );
  }
