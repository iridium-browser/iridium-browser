// facevisitor-windowsfnt.cpp
//
//   Implementation of FaceVisitorWindowsFnt.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-windowsfnt.h"

#include <cassert>

#include <ft2build.h>
#include FT_WINFONTS_H

#include "utils/logging.h"


  void
  freetype::FaceVisitorWindowsFnt::
  run( Unique_FT_Face  face )
  {
    FT_Error             error;
    FT_WinFNT_HeaderRec  header;


    assert( face != nullptr );

    error = FT_Get_WinFNT_Header( face.get(), &header );
    LOG_FT_ERROR( "FT_Get_WinFNT_Header", error );

    LOG_IF( INFO, error == 0) << "retrieved Windows FNT font info header";
  }
