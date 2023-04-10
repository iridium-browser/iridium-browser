// facevisitor-loadglyphs.cpp
//
//   Implementation of FaceVisitorLoadGlyphs.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-loadglyphs.h"

#include <cassert>

#include "utils/logging.h"


  freetype::FaceVisitorLoadGlyphs::
  FaceVisitorLoadGlyphs( FT_Long  num_used_glyphs )
  {
    (void) set_num_used_glyphs( num_used_glyphs );
    (void) add_transformation( nullptr, nullptr );
  }


  void
  freetype::FaceVisitorLoadGlyphs::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;
    FT_Long   num_glyphs;


    assert( face            != nullptr &&
            num_used_glyphs >  0 );

    num_glyphs = face->num_glyphs;

    for ( auto  transformation : transformations )
    {
      FT_Matrix*  matrix = transformation.first;
      FT_Vector*  vector = transformation.second;


      LOG_IF( INFO, matrix == nullptr )
        << "setting transformation matrix: none";
      LOG_IF( INFO, matrix != nullptr )
        << "setting transformation matrix: "
        << matrix->xx << ", " << matrix->xy << "; "
        << matrix->yx << ", " << matrix->yy;

      LOG_IF( INFO, vector == nullptr ) << "setting transform vector: none";
      LOG_IF( INFO, vector != nullptr )
        << "setting transform vector: "
        << vector->x << ", " << vector->y;

      (void) FT_Set_Transform( face.get(), matrix, vector );

      for ( auto  index = 0;
            index < num_glyphs &&
              index < num_used_glyphs;
            index++ )
      {
        LOG( INFO ) << "testing glyph " << ( index + 1 ) << "/" << num_glyphs;

        for ( auto  flags : load_flags )
        {
          LOG( INFO ) << "load flags: 0x" << std::hex << flags;

          error = FT_Load_Glyph( face.get(), index, flags );
          LOG_FT_ERROR( "FT_Load_Glyph", error );
        }
      }

      WARN_ABOUT_IGNORED_VALUES( num_glyphs, num_used_glyphs, "glyphs" );
    }
  }


  void
  freetype::FaceVisitorLoadGlyphs::
  set_num_used_glyphs( FT_Long  glyphs )
  {
    assert( glyphs > 0 );
    num_used_glyphs = glyphs;
  }


  void
  freetype::FaceVisitorLoadGlyphs::
  add_transformation( FT_Matrix*  matrix,
                      FT_Vector*  delta )
  {
    (void) transformations.push_back( { matrix, delta } );
  }


  void
  freetype::FaceVisitorLoadGlyphs::
  add_load_flags( FT_Int32  load_flags )
  {
    (void) this->load_flags.push_back( load_flags );
  }
