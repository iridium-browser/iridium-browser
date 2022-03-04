// glyphvisitor-outlines.cpp
//
//   Implementation of GlyphVisitorOutlines.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/glyphvisitor-outlines.h"

#include <cassert>

#include <ft2build.h>
#include FT_OUTLINE_H

#include "utils/logging.h"


  freetype::GlyphVisitorOutlines::
  GlyphVisitorOutlines()
  {
    (void) callbacks.emplace_back( reverse    );
    (void) callbacks.emplace_back( embolden_1 );
    (void) callbacks.emplace_back( embolden_2 );
    (void) callbacks.emplace_back( embolden_3 );
    (void) callbacks.emplace_back( embolden_4 );
  }


  void
  freetype::GlyphVisitorOutlines::
  run( Unique_FT_Glyph  glyph )
  {
    FT_Error     error;
    FT_Outline   outline;
    FT_Outline*  glyph_outline;


    assert( glyph         != nullptr                 &&
            glyph->format == FT_GLYPH_FORMAT_OUTLINE );

    glyph_outline = &( (FT_OutlineGlyph)glyph.get() )->outline;

    error = FT_Outline_New( glyph->library,
                            glyph_outline->n_points,
                            glyph_outline->n_contours,
                            &outline );
    LOG_FT_ERROR( "FT_Outline_New", error );

    if ( error != 0 )
      return;

    for ( auto  fn : callbacks )
    {
      error = FT_Outline_Copy( glyph_outline, &outline );
      LOG_FT_ERROR( "FT_Outline_Copy", error );

      if ( error != 0 )
        continue;

      (void) fn( outline );
    }

    error = FT_Outline_Done( glyph->library, &outline );
    LOG_FT_ERROR( "FT_Outline_Done", error );
  }


  void
  freetype::GlyphVisitorOutlines::
  reverse( FT_Outline&  outline )
  {
    LOG( INFO ) << "reversing outline";

    (void) FT_Outline_Reverse( &outline );
  }


  void
  freetype::GlyphVisitorOutlines::
  embolden( FT_Outline&  outline,
            FT_Pos       strength )
  {
    FT_Error  error;


    LOG( INFO ) << "embolden outline by " << strength;

    error = FT_Outline_Embolden( &outline, strength );
    LOG_FT_ERROR( "FT_Outline_Embolden", error );
  }


  void
  freetype::GlyphVisitorOutlines::
  embolden( FT_Outline&  outline,
            FT_Pos       xstrength,
            FT_Pos       ystrength )
  {
    FT_Error  error;


    LOG( INFO ) << "embolden outline by "
                << xstrength << " x " << ystrength;

    error = FT_Outline_EmboldenXY( &outline, xstrength, ystrength );
    LOG_FT_ERROR( "FT_Outline_EmboldenXY", error );
  }


  void
  freetype::GlyphVisitorOutlines::
  embolden_1( FT_Outline&  outline )
  {
    // Note: the strength is in 26.6 format.
    (void) embolden( outline, 3 * 64 );
  }


  void
  freetype::GlyphVisitorOutlines::
  embolden_2( FT_Outline&  outline )
  {
    // Note: the strength is in 26.6 format.
    (void) embolden( outline, -2 * 64 );
  }


  void
  freetype::GlyphVisitorOutlines::
  embolden_3( FT_Outline&  outline )
  {
    // Note: the strength is in 26.6 format.
    (void) embolden( outline, 3 * 64, -2 * 64 );
  }

  void
  freetype::GlyphVisitorOutlines::
  embolden_4( FT_Outline&  outline )
  {
    // Note: the strength is in 26.6 format.
    (void) embolden( outline, -2 * 64, 3 * 64 );
  }
