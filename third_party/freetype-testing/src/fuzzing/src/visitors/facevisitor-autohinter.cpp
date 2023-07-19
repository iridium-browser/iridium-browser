// facevisitor-autohinter.cpp
//
//   Implementation of FaceVisitorAutohinter.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-autohinter.h"

#include <cassert>

#include <ft2build.h>
#include FT_MODULE_H 

#include "utils/logging.h"


  void
  freetype::FaceVisitorAutohinter::
  run( Unique_FT_Face  face )
  {
    assert( face != nullptr );

    for ( auto  warping : warpings )
    {
      LOG( INFO ) << ( warping == 1 ? "" : "not " ) << "using warping";

      (void) set_property( face, "warping", &warping );
      (void) load_glyphs( face );
    }

    (void) set_property( face, "warping", &default_warping );
  }


  void
  freetype::FaceVisitorAutohinter::
  set_property( Unique_FT_Face&    face,
                const std::string  property_name,
                const void*        value)
  {
    (void) FT_Property_Set( face->glyph->library,
                            "autofitter",
                            property_name.c_str(),
                            value );
  }


  void
  freetype::FaceVisitorAutohinter::
  load_glyphs( Unique_FT_Face&  face )
  {
    FT_Error  error;
    FT_Long   num_glyphs = face->num_glyphs;


    for ( auto  index = 0;
          index < num_glyphs &&
            index < GLYPH_INDEX_MAX;
          index++ )
    {
      LOG( INFO ) << "testing glyph " << ( index + 1 ) << "/" << num_glyphs;

      error = FT_Load_Glyph( face.get(), index, LOAD_FLAGS );
      LOG_FT_ERROR( "FT_Load_Glyph", error );
    }

    WARN_ABOUT_IGNORED_VALUES( num_glyphs, GLYPH_INDEX_MAX, "glyphs" );
  }
