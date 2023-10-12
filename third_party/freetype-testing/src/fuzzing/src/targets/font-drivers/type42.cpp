// type42.cpp
//
//   Implementation of Type42FuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/type42.h"

#include <utility> // std::move

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator-bitmaps.h"
#include "iterators/faceprepiterator-outlines.h"
#include "visitors/facevisitor-charcodes.h"
#include "visitors/facevisitor-type1tables.h"
#include "visitors/facevisitor-variants.h"


  freetype::Type42FuzzTarget::
  Type42FuzzTarget()
  {
    auto  fli = freetype::make_unique<FaceLoadIterator>();

    auto  fpi_bitmaps  = freetype::make_unique<FacePrepIteratorBitmaps>();
    auto  fpi_outlines = freetype::make_unique<FacePrepIteratorOutlines>();


    // -----------------------------------------------------------------------
    // Face load iterators:

    (void) fli->set_supported_font_format( FaceLoader::FontFormat::TYPE_42 );
    
    (void) fli->add_iterator( std::move( fpi_bitmaps  ) );
    (void) fli->add_iterator( std::move( fpi_outlines ) );
    
    (void) fli
      ->add_once_visitor( freetype::make_unique<FaceVisitorCharCodes>() );
    (void) fli
      ->add_once_visitor( freetype::make_unique<FaceVisitorVariants>() );

    (void) fli
      ->add_always_visitor( freetype::make_unique<FaceVisitorType1Tables>() );

    // -----------------------------------------------------------------------
    // Fuzz target:

    (void) set_iterator( std::move( fli ) );
  }
