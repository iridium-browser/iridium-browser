// glyphvisitor-bitmap-handling.cpp
//
//   Implementation of GlyphVisitorBitmapHandling.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/glyphvisitor-bitmap-handling.h"

#include <cassert>

#include <ft2build.h>
#include FT_BITMAP_H

#include "utils/logging.h"


  const FT_Color
  freetype::GlyphVisitorBitmapHandling::
  COLOUR_PINK =
  {
    192, 168, 0, 128
  };

  // Offset vectors are in 26.6 fixed-point format.

  const FT_Vector
  freetype::GlyphVisitorBitmapHandling::
  SOURCE_OFFSET =
  {
    static_cast<FT_Pos>( 0x40 * -3.5 ),
    static_cast<FT_Pos>( 0x40 *  2.5 )
  };


  void
  freetype::GlyphVisitorBitmapHandling::
  run( Unique_FT_Glyph  glyph )
  {
    FT_Library  library;
    FT_Bitmap   bitmap_1;
    FT_Bitmap   bitmap_2;


    assert( glyph         != nullptr                &&
            glyph->format == FT_GLYPH_FORMAT_BITMAP );

    library = glyph->library;

    (void) FT_Bitmap_Init( &bitmap_1 );
    (void) FT_Bitmap_Init( &bitmap_2 );

    if ( extract_bitmap( glyph, bitmap_1 ) == false )
      return;

    // Note: "The current implementation restricts `xStrength' to be less than
    // or equal to 8 if bitmap is of pixel_mode `FT_PIXEL_MODE_MONO'."
    
    // Embolden slightly (stay <= 8):
    (void) embolden( library, bitmap_1, 8, 7 );

    // Blend onto new bitmap:
    (void) blend( library, bitmap_1, bitmap_2 );

    if ( extract_bitmap( glyph, bitmap_1 ) == false )
      return;

    if ( bitmap_1.pixel_mode != FT_PIXEL_MODE_MONO )
      // Embolden big-time:
      (void) embolden( library, bitmap_1, bitmap_1.width, bitmap_1.rows );

    // Blend onto used bitmap:
    (void) blend( library, bitmap_1, bitmap_2 );

    (void) convert( glyph, bitmap_1,  1 );
    (void) convert( glyph, bitmap_1,  2 );
    (void) convert( glyph, bitmap_1,  4 );
    (void) convert( glyph, bitmap_1,  8 );
    (void) convert( glyph, bitmap_1, 32 );

    (void) FT_Bitmap_Done( library, &bitmap_1 );
    (void) FT_Bitmap_Done( library, &bitmap_2 );
  }


  bool
  freetype::GlyphVisitorBitmapHandling::
  extract_bitmap( Unique_FT_Glyph&  glyph,
                  FT_Bitmap&        bitmap )
  {
    return copy_bitmap( glyph->library,
                        ( (FT_BitmapGlyph)glyph.get() )->bitmap,
                        bitmap );
  }


  bool
  freetype::GlyphVisitorBitmapHandling::
  copy_bitmap( FT_Library        library,
               const FT_Bitmap&  source,
               FT_Bitmap&        target )
  {
    FT_Error  error;


    error = FT_Bitmap_Copy( library, &source, &target );
    LOG_FT_ERROR( "FT_Bitmap_Copy", error );

    return error == 0 ? true : false;
  }


  void
  freetype::GlyphVisitorBitmapHandling::
  embolden( FT_Library  library,
            FT_Bitmap&  bitmap,
            FT_Pos      xStrength,
            FT_Pos      yStrength )
  {
    FT_Error  error;


    LOG( INFO ) << "embolden bitmap: " << xStrength << " x " << yStrength;

    error = FT_Bitmap_Embolden( library, &bitmap, xStrength, yStrength );
    LOG_FT_ERROR( "FT_Bitmap_Embolden", error );
  }


  void
  freetype::GlyphVisitorBitmapHandling::
  blend( FT_Library        library,
         const FT_Bitmap&  source,
         FT_Bitmap&        target )
  {
    FT_Error  error;

    // Offset vectors are in 26.6 fixed-point format.
    FT_Vector  target_offset =
      {
        static_cast<FT_Pos>( 0x40 *  4 ),
        static_cast<FT_Pos>( 0x40 * -3 )
      };


    LOG( INFO ) << "blending bitmaps";

    error = FT_Bitmap_Blend( library,
                             &source,
                             SOURCE_OFFSET,
                             &target,
                             &target_offset,
                             COLOUR_PINK );
    LOG_FT_ERROR( "FT_Bitmap_Blend", error );
  }

    
  void
  freetype::GlyphVisitorBitmapHandling::
  convert( Unique_FT_Glyph&  glyph,
           FT_Bitmap&        bitmap,
           FT_Int            alignment )
  {
    FT_Error  error;


    LOG( INFO ) << "converting bitmap: " << alignment;

    error = FT_Bitmap_Convert( glyph->library,
                               &( (FT_BitmapGlyph)glyph.get() )->bitmap,
                               &bitmap,
                               alignment );
    LOG_FT_ERROR( "FT_Bitmap_Convert", error );
  }
