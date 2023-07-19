// facevisitor-loadglyphs-outlines.cpp
//
//   Implementation of FaceVisitorLoadGlyphsOutlines.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-loadglyphs-outlines.h"


  const FT_Long
  freetype::FaceVisitorLoadGlyphsOutlines::
  NUM_USED_GLYPHS = 5;


  freetype::FaceVisitorLoadGlyphsOutlines::
  FaceVisitorLoadGlyphsOutlines()
    : FaceVisitorLoadGlyphs( NUM_USED_GLYPHS )
  {
    // Notes:
    //   - The auto-hinter has its own visitor.
    //   - Rendering has its own visitor for performance reasons.

    FT_Int32  flags = FT_LOAD_NO_BITMAP | FT_LOAD_NO_AUTOHINT;


    // Rotate by 3 degrees.
    // Coefficients are in 16.16 fixed-point format.
    matrix.xx = 0x10000L *  0.99862;
    matrix.xy = 0x10000L * -0.05233;
    matrix.yx = 0x10000L *  0.05233;
    matrix.yy = 0x10000L *  0.99862;

    // Coordinates are expressed in 1/64th of a pixel.
    delta.x = -3 * 64;
    delta.y =  3 * 64;

    (void) add_transformation( nullptr, &delta  );
    (void) add_transformation( &matrix, nullptr );
    (void) add_transformation( &matrix, &delta  );

    (void) add_load_flags( flags                           );
    (void) add_load_flags( flags | FT_LOAD_NO_SCALE        );
    (void) add_load_flags( flags | FT_LOAD_NO_HINTING      );
    (void) add_load_flags( flags | FT_LOAD_VERTICAL_LAYOUT );
    (void) add_load_flags( flags | FT_LOAD_PEDANTIC        );
    (void) add_load_flags( flags | FT_LOAD_LINEAR_DESIGN   );
    (void) add_load_flags( flags | FT_LOAD_COMPUTE_METRICS );
  }
