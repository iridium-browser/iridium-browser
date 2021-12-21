// FuzzTarget.cpp
//
//   Implementation of FuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/FuzzTarget.h"

#include "utils/logging.h"


  freetype::FuzzTarget::
  FuzzTarget()
  {
    FT_Error  error;

    FT_Int  major;
    FT_Int  minor;
    FT_Int  patch;

    
    error = FT_Init_FreeType( &library );
    LOG_FT_ERROR( "FT_Init_FreeType", error );

    if ( error != 0 )
      return;

    (void) FT_Library_Version( library, &major, &minor, &patch );
    LOG( INFO ) << "Using FreeType " << major << "." << minor << "." << patch;
  }


  freetype::FuzzTarget::
  ~FuzzTarget()
  {
    (void) FT_Done_FreeType( library );
    library = nullptr;
  }
