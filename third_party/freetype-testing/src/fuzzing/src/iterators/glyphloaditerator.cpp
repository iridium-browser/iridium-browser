// glyphloaditerator.cpp
//
//   Implementation of GlyphLoadIterator.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "iterators/glyphloaditerator.h"

#include "utils/logging.h"


  freetype::GlyphLoadIterator::
  GlyphLoadIterator( FT_Long  num_load_glyphs )
  {
    (void) set_num_load_glyphs( num_load_glyphs );
  }


  void
  freetype::GlyphLoadIterator::
  set_num_load_glyphs( FT_Long  glyphs )
  {
    num_load_glyphs = glyphs;
    LOG( INFO ) << "num glyphs: " << num_load_glyphs;
  }


  void
  freetype::GlyphLoadIterator::
  add_load_flags( FT_Int32  flags )
  {
    load_flags |= flags;
  }


  void
  freetype::GlyphLoadIterator::
  add_visitor( std::unique_ptr<GlyphVisitor>  visitor )
  {
    (void) glyph_visitors.emplace_back( std::move( visitor ) );
  }


  void
  freetype::GlyphLoadIterator::
  add_iterator( std::unique_ptr<GlyphRenderIterator>  iterator )
  {
    (void) glyph_render_iterators.emplace_back( std::move( iterator ) );
  }


  void
  freetype::GlyphLoadIterator::
  invoke_visitors_and_iterators( const Unique_FT_Glyph&  glyph )
  {
    Unique_FT_Glyph  buffer_glyph = make_unique_glyph();


    for ( auto&  visitor : glyph_visitors )
    {
      buffer_glyph = copy_unique_glyph( glyph );
      if ( buffer_glyph == nullptr )
        return; // we can expect this to fail again; bail out!
      
      visitor->run( std::move( buffer_glyph ) );
    }

    for ( auto&  iterator : glyph_render_iterators )
    {
      buffer_glyph = copy_unique_glyph( glyph );
      if ( buffer_glyph == nullptr )
        return; // we can expect this to fail again; bail out!
      
      iterator->run( std::move( buffer_glyph ) );
    }
  }
