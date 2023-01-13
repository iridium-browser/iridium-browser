// cidtype1-render.cpp
//
//   Implementation of CidType1RenderFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/cidtype1-render.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "iterators/faceprepiterator-outlines.h"
#include "visitors/facevisitor-autohinter.h"
#include "visitors/facevisitor-loadglyphs-bitmaps.h"
#include "visitors/facevisitor-loadglyphs-outlines.h"
#include "visitors/facevisitor-renderglyphs.h"
#include "visitors/facevisitor-subglyphs.h"


  freetype::CidType1RenderFuzzTarget::
  CidType1RenderFuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();

    auto  fpi_bitmaps  = freetype::make_unique<FacePrepIteratorBitmaps>();
    auto  fpi_outlines = freetype::make_unique<FacePrepIteratorOutlines>();


    // -----------------------------------------------------------------------
    // Face preparation iterators:

    (void) fpi_bitmaps
      ->add_visitor( freetype::make_unique<FaceVisitorLoadGlyphsBitmaps>() );

    (void) fpi_outlines
      ->add_visitor( freetype::make_unique<FaceVisitorAutohinter>() );
    (void) fpi_outlines
      ->add_visitor( freetype::make_unique<FaceVisitorLoadGlyphsOutlines>() );
    (void) fpi_outlines
      ->add_visitor( freetype::make_unique<FaceVisitorRenderGlyphs>() );
    (void) fpi_outlines
      ->add_visitor( freetype::make_unique<FaceVisitorSubGlyphs>() );

    // -----------------------------------------------------------------------
    // Face load iterators:

    (void) fli
      ->set_supported_font_format( FaceLoader::FontFormat::CID_TYPE_1 );

    (void) fli->add_iterator( std::move( fpi_bitmaps  ) );
    (void) fli->add_iterator( std::move( fpi_outlines ) );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_property( "t1cid", "hinting-engine", &HINTING_ADOBE );

    (void) set_iterator( std::move( fli ) );
  }
