// facevisitor-kerning.cpp
//
//   Implementation of FaceVisitorKerning.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-kerning.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::FaceVisitorKerning::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_Long    num_glyphs;
    FT_Vector  kerning;
      

    assert( face != nullptr );

    if ( FT_HAS_KERNING( face ) == 0 )
      return;

    num_glyphs = face->num_glyphs;

    for ( auto left_glyph = 0;
          left_glyph < num_glyphs - 1 &&
            left_glyph < MAX_GLYPH_INDEX - 1;
          left_glyph++ )
    {
      for ( auto right_glyph = left_glyph + 1;
           right_glyph < num_glyphs &&
             right_glyph < MAX_GLYPH_INDEX - 1;
           right_glyph++ )
      {
        LOG( INFO ) << "get kerning for glyphs: "
                    << left_glyph << "/" << num_glyphs << ", "
                    << right_glyph << "/" << num_glyphs;

        for ( auto kern_mode = 0;
              kern_mode < FT_KERNING_UNSCALED;
              kern_mode++ )
        {
          error = FT_Get_Kerning( face.get(),
                                  left_glyph,
                                  right_glyph,
                                  kern_mode,
                                  &kerning );
          LOG_FT_ERROR( "FT_Get_Kerning", error );

          error = FT_Get_Kerning( face.get(),
                                  right_glyph,
                                  left_glyph,
                                  kern_mode,
                                  &kerning );
          LOG_FT_ERROR( "FT_Get_Kerning", error );
        }
      }
    }

    WARN_ABOUT_IGNORED_VALUES( num_glyphs, MAX_GLYPH_INDEX, "glyphs" );
  }
