// LzwFuzzTarget.cpp
//
//   Implementation of LzwFuzzTarget.
//
// Copyright 2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/support/LzwFuzzTarget.h"

#include FT_LZW_H

#include "utils/FreeTypeStream.h"
#include "utils/logging.h"


  void
  freetype::LzwFuzzTarget::
  run( const uint8_t*  data,
       size_t          size )
  {
    FreeTypeStream  compressed( get_ft_memory(), data, size );
    FreeTypeStream  uncompressed;
    FT_Error        error;


    error = FT_Stream_OpenLZW( uncompressed.get(), compressed.get() );
    LOG_FT_ERROR( "FT_Stream_OpenLZW", error );

    if ( error != 0 )
      return;

    (void) uncompressed.read_forwards();
    (void) uncompressed.read_backwards();
  }
