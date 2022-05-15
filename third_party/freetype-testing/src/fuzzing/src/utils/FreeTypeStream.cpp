// FreeTypeStream.cpp
//
//   Implementation of FreeTypeStream.
//
// Copyright 2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "utils/FreeTypeStream.h"

#include "utils/logging.h"


  freetype::FreeTypeStream::
  FreeTypeStream( FT_Memory       memory,
                  const FT_Byte*  base,
                  FT_ULong        size )
  {
    (void) FT_Stream_OpenMemory( &stream, base, size );
    stream.memory = memory;
  }


  FT_Error
  freetype::FreeTypeStream::
  seek_relative( ssize_t  offset )
  {
    FT_Error  error;


    // Note that `FT_Stream_Skip' does not accept negative distances so we
    // have to go via `FT_Stream_Pos' and `FT_Stream_Seek'.

    error = FT_Stream_Seek( &stream,
                            FT_Stream_Pos( &stream ) + offset );
    LOG_FT_ERROR( "FT_Stream_Seek", error );

    return error;
  }


  void
  freetype::FreeTypeStream::
  read_forwards( size_t  max_steps )
  {
    FT_Error  error = FT_Err_Ok;
    size_t    count = 0;
    FT_Byte   buffer;


    while ( max_steps-- > 0 )
    {
      if ( FT_Stream_TryRead( &stream, &buffer, 1 ) == 0 )
        break;

      ++count;
    }

    if ( FT_Stream_Pos( &stream) > 0 )
      // `FT_Stream_TryRead' puts the cursor one past the end IF the stream
      // contains at least one byte ...
      (void) seek_relative( -1 );

    LOG( INFO ) << "Read " << count << " bytes (forwards)";
  }


  void
  freetype::FreeTypeStream::
  read_backwards( size_t  max_steps )
  {
    FT_Error  error = FT_Err_Ok;
    size_t    count = 0;
    FT_Byte   buffer;


    while ( max_steps-- > 0 ) {

      if ( FT_Stream_TryRead( &stream, &buffer, 1 ) == 0 )
        break;

      ++count;

      if ( FT_Stream_Pos( &stream ) == 1 )
        break;

      // Step back twice to mitigate the automatic step forward by
      // `FT_Stream_TryRead'.
      error = seek_relative( -2 );
      LOG_FT_ERROR( "seek_relative", error );

      if ( error != FT_Err_Ok )
        break;
    }

    LOG( INFO ) << "Read " << count << " bytes (backwards)";
  }
