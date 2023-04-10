// utils.cpp
//
//   Implementation of utils.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "utils/utils.h"

#include <cassert>
#include <cmath>
#include <limits>

#include <ft2build.h>
#include FT_GLYPH_H

#include "utils/logging.h"


  freetype::Unique_FT_Face
  freetype::
  make_unique_face( FT_Face  face )
  {
    return Unique_FT_Face( face, FT_Done_Face );
  }


  freetype::Unique_FT_Glyph
  freetype::
  make_unique_glyph( FT_Glyph  glyph )
  {
    return Unique_FT_Glyph( glyph, FT_Done_Glyph );
  }


  freetype::Unique_FT_Glyph
  freetype::
  copy_unique_glyph( const Unique_FT_Glyph&  glyph )
  {
    FT_Error  error;
    FT_Glyph  raw_glyph;


    error = FT_Glyph_Copy( glyph.get(), &raw_glyph );
    LOG_FT_ERROR( "FT_Glyph_Copy", error );

    return make_unique_glyph( error == 0 ? raw_glyph : nullptr );
  }


  freetype::Unique_FT_Glyph
  freetype::
  get_unique_glyph_from_face( const Unique_FT_Face&  face )
  {
    FT_Error  error;
    FT_Glyph  glyph;


    error = FT_Get_Glyph( face->glyph, &glyph );
    LOG_FT_ERROR( "FT_Get_Glyph", error );

    return make_unique_glyph( error == 0 ? glyph : nullptr );
  }


  bool
  freetype::
  glyph_has_reasonable_size( const Unique_FT_Glyph&  glyph,
                             FT_Pos                  reasonable_pixels )
  {
    return glyph_has_reasonable_size( glyph, reasonable_pixels, 0, 0 );
  }


  bool
  freetype::
  glyph_has_reasonable_size( const Unique_FT_Glyph&  glyph,
                             FT_Pos                  reasonable_pixels,
                             FT_Pos                  reasonable_width,
                             FT_Pos                  reasonable_height )
  {
    static const auto  POS_MAX = std::numeric_limits<FT_Pos>::max();

    FT_BBox  box;
    FT_Pos   pixels;
    FT_Pos   width;
    FT_Pos   height;


    if ( glyph == nullptr )
    {
      LOG( WARNING ) << "glyph is null";
      return false;
    }

    (void) FT_Glyph_Get_CBox( glyph.get(), FT_GLYPH_BBOX_PIXELS, &box );

    width  = std::abs( box.xMin - box.xMax );
    height = std::abs( box.yMin - box.yMax );

    LOG( INFO ) << "glyph size: " << width << " x " << height << " px\n";

    if ( ( reasonable_width  > 0 && width  > reasonable_width  ) ||
         ( reasonable_height > 0 && height > reasonable_height ) )
    {
      LOG( WARNING ) << "glyph is beyond reasonbale size";
      return false;
    }

    // Beware possible overflows:
    pixels = width > 0 && POS_MAX / width < height ? POS_MAX : width * height;

    if ( reasonable_pixels > 0 && pixels > reasonable_pixels )
    {
      LOG( WARNING ) << "glyph is beyond reasonbale size";
      return false;
    }

    return true;
  }


  bool
  freetype::
  glyph_has_reasonable_work_size( const Unique_FT_Glyph&  glyph )
  {
    // August 2018:
    //   32767 x 32767 pixels is a good size to work on glyphs (when NOT
    //   rendering them).  It's definitely more than what is needed in
    //   regular, real-life scenarios.

    return glyph_has_reasonable_size( glyph, 32767 * 32767 );
  }


  bool
  freetype::
  glyph_has_reasonable_render_size( const Unique_FT_Glyph&  glyph )
  {
    // August 2018:
    //   2500 x 2500 pixels a bit restrictive upper bound to render glyphs.
    //   However, rendering larger glyphs than that starts to take up
    //   considerable amount of time which is precious when rendering a few
    //   hundred (up to a few thousand) glyphs within a few seconds.
    //   Special stress targets could be developed that specialise on glyphs
    //   up to 32767 x 32767 pixels.
    //   In addition, limit the width and height since at least the width
    //   has a big influence on the performance.

    // November 2018:
    //   The heights have to be restricted even further; see
    //   https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=9930.

    return glyph_has_reasonable_size( glyph, 2500 * 2500, 10000, 5000 );
  }
