// pcf-render.cpp
//
//   Implementation of PcfRenderFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/pcf-render.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "visitors/facevisitor-loadglyphs-bitmaps.h"


  freetype::PcfRenderFuzzTarget::
  PcfRenderFuzzTarget()
  {
    auto  fli          = freetype::make_unique<FaceLoadIterator>();
    auto  fpi_bitmaps  = freetype::make_unique<FacePrepIteratorBitmaps>();


    // -----------------------------------------------------------------------
    // Face preparation iterator:

    (void) fpi_bitmaps
      ->add_visitor( freetype::make_unique<FaceVisitorLoadGlyphsBitmaps>() );

    // -----------------------------------------------------------------------
    // Face load iterator:

    (void) fli->set_supported_font_format( FaceLoader::FontFormat::PCF );
    
    (void) fli->add_iterator( std::move( fpi_bitmaps ) );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_iterator( std::move( fli ) );
  }
