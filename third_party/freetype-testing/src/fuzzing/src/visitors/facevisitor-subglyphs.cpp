// facevisitor-subglyphs.cpp
//
//   Implementation of FaceVisitorSubGlyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-subglyphs.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::FaceVisitorSubGlyphs::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_Long  num_glyphs;
    FT_UInt  num_subglyphs;

    FT_Int     sg_index;
    FT_UInt    sg_flags;
    FT_Int     sg_arg1;
    FT_Int     sg_arg2;
    FT_Matrix  sg_transform;


    assert( face != nullptr );

    num_glyphs = face->num_glyphs;

    for ( auto  index = 0;
          index < num_glyphs &&
            index < GLYPH_INDEX_MAX;
          index++ )
    {
      LOG( INFO ) << "testing glyph " << ( index + 1 ) << "/" << num_glyphs;

      error = FT_Load_Glyph( face.get(), index, LOAD_FLAGS );
      LOG_FT_ERROR( "FT_Load_Glyph", error );

      if ( error != 0 )
        continue;

      if ( face->glyph->format != FT_GLYPH_FORMAT_COMPOSITE )
      {
        LOG( INFO ) << "no composite glyph found";
        continue;
      }

      num_subglyphs = face->glyph->num_subglyphs;

      for ( FT_UInt sub_index = 0;
            sub_index < num_subglyphs &&
              sub_index < SUBGLYPH_INDEX_MAX;
            sub_index++ )
      {
        error = FT_Get_SubGlyph_Info( face->glyph,
                                      sub_index,
                                      &sg_index,
                                      &sg_flags,
                                      &sg_arg1,
                                      &sg_arg2,
                                      &sg_transform );
        LOG_FT_ERROR( "FT_Get_SubGlyph_Info", error );

        LOG_IF( INFO, error == 0 )
          << "subglyph " << ( sub_index + 1 ) << "/" << num_subglyphs << ": "
          << "glyph #" << sg_index << ", "
          << "flags 0x" << std::hex << sg_flags;
      }

      WARN_ABOUT_IGNORED_VALUES( num_subglyphs,
                                 SUBGLYPH_INDEX_MAX,
                                 "subglyphs" );
    }

    WARN_ABOUT_IGNORED_VALUES( num_glyphs, GLYPH_INDEX_MAX, "glyphs" );
  }
