// FreeTypeStream.h
//
//   Handy wrapper of `FT_Stream'.
//
// Copyright 2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef UTILS_FREETYPESTREAM_H_
#define UTILS_FREETYPESTREAM_H_


#include <cstdint>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/internal/ftstream.h>


#include "utils/noncopyable.h"


namespace freetype {


  class FreeTypeStream
    : private noncopyable
  {
  public:


    FreeTypeStream() = default;


    FreeTypeStream( FT_Memory       memory,
                    const FT_Byte*  base,
                    FT_ULong        size );


    virtual
    ~FreeTypeStream()
    {
      (void) FT_Stream_Close( &stream );
    }


    FT_Stream
    get()
    {
      return &stream;
    }


    FT_Error
    seek_relative( ssize_t  offset );


    void
    read_forwards( size_t  max_steps = 500 );


    void
    read_backwards( size_t  max_steps = 500 );


  private:


    FT_StreamRec  stream = {};
  };
}


#endif // UTILS_FREETYPESTREAM_H_
