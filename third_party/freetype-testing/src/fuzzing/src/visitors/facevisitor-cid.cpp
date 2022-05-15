// facevisitor-cid.cpp
//
//   Implementation of FaceVisitorCid.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-cid.h"

#include <cassert>

#include <ft2build.h>
#include FT_CID_H

#include "utils/logging.h"


  void
  freetype::FaceVisitorCid::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_Long  num_glyphs;

    const char*  registry;
    const char*  ordering;
    FT_Int       supplement;

    FT_Bool  is_cid;

    FT_UInt  cid;


    assert( face != nullptr );

    num_glyphs = face->num_glyphs;

    error = FT_Get_CID_Registry_Ordering_Supplement( face.get(),
                                                     &registry,
                                                     &ordering,
                                                     &supplement );
    LOG_FT_ERROR( "FT_Get_CID_Registry_Ordering_Supplement", error );

    LOG_IF( INFO, error == 0 ) << "cid r/o/s: "
                               << registry << "/"
                               << ordering << "/"
                               << supplement;

    error = FT_Get_CID_Is_Internally_CID_Keyed( face.get(), &is_cid );
    LOG_FT_ERROR( "FT_Get_CID_Is_Internally_CID_Keyed", error );

    LOG_IF( INFO, error == 0 ) << "cid is "
                               << ( is_cid == 0 ? "not " : "" )
                               << "internally cid keyed";

    for ( auto  index = 0;
          index < num_glyphs &&
            index < GLYPH_INDEX_MAX;
          index++ )
    {
      error = FT_Get_CID_From_Glyph_Index( face.get(), index, &cid );
      LOG_FT_ERROR( "FT_Get_CID_From_Glyph_Index", error );

      if ( error != 0)
        break; // we can expect this call to fail again.

      LOG_IF( INFO, error == 0 ) << index << "/" << num_glyphs
                                 << " (glyph index) <-> "
                                 << cid   << " (cid)";
    }

    WARN_ABOUT_IGNORED_VALUES( num_glyphs, GLYPH_INDEX_MAX, "glyphs" );
  }
