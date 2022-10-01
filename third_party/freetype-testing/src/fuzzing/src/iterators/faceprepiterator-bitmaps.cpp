// faceprepiterator-bitmaps.cpp
//
//   Implementation of FacePrepIteratorBitmaps.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "iterators/faceprepiterator-bitmaps.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::FacePrepIteratorBitmaps::
  run( const std::unique_ptr<FaceLoader>&  face_loader )
  {
    Unique_FT_Face  face = make_unique_face();
    FT_Int          num_strikes;


    assert( face_loader != nullptr );

    face = face_loader->load();

    if ( face == nullptr )
    {
      LOG( ERROR ) << "face_loader->load() failed";
      return;
    }

    if ( face->style_flags >> 16 != 0 )
      return;
    
    num_strikes = face->num_fixed_sizes;

    for ( auto  index = 0;
          index < num_strikes &&
            index < STRIKE_INDEX_MAX;
          index++ )
    {
      LOG( INFO )
        << "using bitmap strike "
        << ( index + 1 ) << "/" << num_strikes;

      for ( auto&  visitor : face_visitors )
        visitor->run( get_prepared_face( face_loader, index ) );
      
      for ( auto&  iterator : glyph_load_iterators )
        iterator->run( get_prepared_face( face_loader, index ) );
    }

    WARN_ABOUT_IGNORED_VALUES( num_strikes,
                               STRIKE_INDEX_MAX,
                               "bitmap strikes" );
  }


  freetype::Unique_FT_Face
  freetype::FacePrepIteratorBitmaps::
  get_prepared_face( const std::unique_ptr<FaceLoader>&  face_loader,
                     FT_Int                              index )
  {
    FT_Error  error;

    Unique_FT_Face  face = face_loader->load();


    if ( face == nullptr )
    {
      LOG( ERROR ) << "face_loader->load() failed";
      return make_unique_face();
    }

    error = FT_Select_Size( face.get(), index );
    LOG_FT_ERROR( "FT_Select_Size", error );
    
    return error == 0 ? std::move( face ) : make_unique_face();
  }
