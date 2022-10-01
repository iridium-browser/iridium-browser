// faceprepiterator-outlines.cpp
//
//   Implementation of FacePrepIteratorOutlines.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "iterators/faceprepiterator-outlines.h"

#include <memory> // std::unique_ptr

#include "utils/logging.h"


  freetype::FacePrepIteratorOutlines::
  FacePrepIteratorOutlines()
  {
    // Arbitrary char sizes:
    (void) append_char_size( 16, 16,  8,  8,  72,  72 );
    (void) append_char_size( 16, 32,  8, 20,  72, 300 );
    (void) append_char_size(  0, 32,  0, 20,   0,  72 );
    (void) append_char_size( 32,  0, 64,  0, 300,   0 );
  }


  void
  freetype::FacePrepIteratorOutlines::
  run( const std::unique_ptr<FaceLoader>&  face_loader )
  {
    for ( size_t index = 0; index < char_sizes.size(); index++ )
    {
      LOG( INFO )
        << "using char size "
        << ( index + 1 ) << "/" << char_sizes.size() << ": "
        << std::get<0>( char_sizes[index] ) << " x "
        << std::get<1>( char_sizes[index] ) << " ppem, "
        << ( std::get<2>( char_sizes[index] ) / 64 ) << " x "
        << ( std::get<3>( char_sizes[index] ) / 64 ) << " pt, "
        << std::get<4>( char_sizes[index] ) << " x "
        << std::get<5>( char_sizes[index] ) << " dpi";

      // Test a face + ignore it if it cannot be loaded:
      if ( get_prepared_face( face_loader, index ) == nullptr )
        continue;
      
      for ( auto&  visitor : face_visitors )
        visitor->run( get_prepared_face( face_loader, index ) );
      
      for ( auto&  iterator : glyph_load_iterators )
        iterator->run( get_prepared_face( face_loader, index ) );
    }
  }


  freetype::Unique_FT_Face
  freetype::FacePrepIteratorOutlines::
  get_prepared_face( const std::unique_ptr<FaceLoader>&  face_loader,
                     CharSizeTuples::size_type           index )
  {
    FT_Error  error;

    auto  face = face_loader->load();

    
    if ( face == nullptr )
    {
      LOG( ERROR ) << "face_loader->load() failed";
      return make_unique_face();
    }

    error = FT_Set_Pixel_Sizes( face.get(),
                                std::get<0>( char_sizes[index] ),
                                std::get<1>( char_sizes[index] ) );
    LOG_FT_ERROR( "FT_Set_Pixel_Sizes", error );

    if ( error != 0 )
      return make_unique_face();

    error = FT_Set_Char_Size( face.get(),
                              std::get<2>( char_sizes[index] ),
                              std::get<3>( char_sizes[index] ),
                              std::get<4>( char_sizes[index] ),
                              std::get<5>( char_sizes[index] ) );
    LOG_FT_ERROR( "FT_Set_Char_Size", error );

    return error == 0 ? std::move( face ) : make_unique_face();
  }


  void
  freetype::FacePrepIteratorOutlines::
  append_char_size( FT_UInt     pixel_width_ppem,
                    FT_UInt     pixel_height_ppem,
                    FT_F26Dot6  char_width_pt,
                    FT_F26Dot6  char_height_pt,
                    FT_UInt     horz_resolution_dpi,
                    FT_UInt     vert_resolution_dpi )
  {
    // Note: FreeType expects char size as 1/64th of points.
    
    (void) char_sizes.emplace_back(
      std::make_tuple ( pixel_width_ppem,
                        pixel_height_ppem,
                        char_width_pt  * 64,
                        char_height_pt * 64,
                        horz_resolution_dpi,
                        vert_resolution_dpi ) );
  }
