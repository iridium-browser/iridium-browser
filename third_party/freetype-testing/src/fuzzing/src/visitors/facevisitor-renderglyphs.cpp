// facevisitor-renderglyphs.cpp
//
//   Implementation of FaceVisitorRenderGlyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-renderglyphs.h"

#include <cassert>

#include "utils/logging.h"


  const freetype::FaceVisitorRenderGlyphs::RenderModes
  freetype::FaceVisitorRenderGlyphs::
  RENDER_MODES =
  {
    { FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_NORMAL, FT_RENDER_MODE_NORMAL },
    { FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LIGHT,  FT_RENDER_MODE_LIGHT  },
    { FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_MONO,   FT_RENDER_MODE_MONO   },
    { FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LCD,    FT_RENDER_MODE_LCD    },
    { FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_LCD_V,  FT_RENDER_MODE_LCD_V  }
  };


  void
  freetype::FaceVisitorRenderGlyphs::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;
    FT_Long   num_glyphs;


    assert( face != nullptr );

    num_glyphs = face->num_glyphs;

    for ( auto  index = 0;
          index < num_glyphs &&
            index < GLYPH_INDEX_MAX;
          index++ )
    {
      LOG( INFO ) << "testing glyph " << ( index + 1 ) << "/" << num_glyphs;

      for ( auto  mode : RENDER_MODES )
      {
        LOG( INFO ) << "load flags: 0x" << std::hex << mode.first;

        error = FT_Load_Glyph( face.get(), index, mode.first );
        LOG_FT_ERROR( "FT_Load_Glyph", error );

        if ( error != 0 )
          continue;

        if ( glyph_has_reasonable_render_size(
               get_unique_glyph_from_face( face ) ) == false )
          continue;

        LOG( INFO ) << "render mode: " << mode.second;

        error = FT_Render_Glyph( face->glyph, mode.second );
        LOG_FT_ERROR( "FT_Render_Glyph", error );
      }
    }

    WARN_ABOUT_IGNORED_VALUES( num_glyphs, GLYPH_INDEX_MAX, "glyphs" );
  }
