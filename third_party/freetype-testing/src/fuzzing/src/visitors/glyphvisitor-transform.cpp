// glyphvisitor-transform.cpp
//
//   Implementation of GlyphVisitorTransform.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/glyphvisitor-transform.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::GlyphVisitorTransform::
  run( Unique_FT_Glyph  glyph )
  {
    FT_Matrix  matrix;
    FT_Vector  delta;

    
    assert( glyph != nullptr );

    // Rotate by 3°.
    // Coefficients are in 16.16 fixed-point format.
    matrix.xx = 0x10000L *  0.99862;
    matrix.xy = 0x10000L * -0.05233;
    matrix.yx = 0x10000L *  0.05233;
    matrix.yy = 0x10000L *  0.99862;

    // Coordinates are expressed in 1/64th of a pixel.
    delta.x = -3 * 64;
    delta.y =  3 * 64;

    LOG( INFO ) << "transforming glyph";

    (void) transform( glyph, nullptr, nullptr );
    (void) transform( glyph, nullptr, &delta );
    (void) transform( glyph, &matrix, nullptr );
    (void) transform( glyph, &matrix, &delta );
  }


  void
  freetype::GlyphVisitorTransform::
  transform( const Unique_FT_Glyph&  glyph,
             FT_Matrix*              matrix,
             FT_Vector*              delta )
  {
    FT_Error  error;


    error = FT_Glyph_Transform( glyph.get(), matrix, delta );
    LOG_FT_ERROR( "FT_Glyph_Transform", error );
  }
