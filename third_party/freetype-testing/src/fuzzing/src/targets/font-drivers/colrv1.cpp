// colrv1.cpp
//
//   Implementation of ColrV1FuzzTarget.
//
// Copyright 2021 by
// Dominik Röttsches, Armin Hasitzka, David Turner, Robert Wilhelm,
// and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/colrv1.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "iterators/faceprepiterator-outlines.h"
#include "visitors/facevisitor-colrv1.h"


  freetype::ColrV1FuzzTarget::
  ColrV1FuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();


    // -----------------------------------------------------------------------
    // Face load iterators:

    (void) fli->set_supported_font_format( FaceLoader::FontFormat::TRUETYPE );

    (void) fli
      ->add_once_visitor( freetype::make_unique<FaceVisitorColrV1>() );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_iterator( std::move( fli ) );
  }
