// cidtype1.cpp
//
//   Implementation of CidType1FuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/cidtype1.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "iterators/faceprepiterator-outlines.h"
#include "visitors/facevisitor-cid.h"
#include "visitors/facevisitor-charcodes.h"
#include "visitors/facevisitor-type1tables.h"
#include "visitors/facevisitor-variants.h"


  freetype::CidType1FuzzTarget::
  CidType1FuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();

    auto  fpi_bitmaps  = freetype::make_unique<FacePrepIteratorBitmaps>();
    auto  fpi_outlines = freetype::make_unique<FacePrepIteratorOutlines>();


    // -----------------------------------------------------------------------
    // Face load iterators:

    (void) fli
      ->set_supported_font_format( FaceLoader::FontFormat::CID_TYPE_1 );

    (void) fli->add_iterator( std::move( fpi_bitmaps  ) );
    (void) fli->add_iterator( std::move( fpi_outlines ) );

    (void) fli->add_once_visitor( freetype::make_unique<FaceVisitorCid>() );
    (void) fli
      ->add_once_visitor( freetype::make_unique<FaceVisitorCharCodes>() );
    (void) fli
      ->add_once_visitor( freetype::make_unique<FaceVisitorVariants>() );

    (void) fli
      ->add_always_visitor( freetype::make_unique<FaceVisitorType1Tables>() );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_property( "t1cid", "hinting-engine", &HINTING_ADOBE );

    (void) set_iterator( std::move( fli ) );
  }
