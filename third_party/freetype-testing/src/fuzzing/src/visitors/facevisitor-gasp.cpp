// facevisitor-gasp.cpp
//
//   Implementation of FaceVisitorGasp.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-gasp.h"

#include <cassert>

#include <ft2build.h>
#include FT_GASP_H

#include "utils/logging.h"


  void
  freetype::FaceVisitorGasp::
  run( Unique_FT_Face  face )
  {
    FT_Int  flags;


    assert( face != nullptr );

    for ( auto  ppem : ppems )
    {
      flags = FT_Get_Gasp( face.get(), ppem );
      LOG( INFO ) << "gasp: " << std::hex << "0x" << flags;
    }
  }
