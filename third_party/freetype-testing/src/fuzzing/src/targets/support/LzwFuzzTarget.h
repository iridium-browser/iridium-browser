// LzwFuzzTarget.h
//
//   Fuzz target for `FT_*LZW*'.
//
// Copyright 2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_SUPPORT_LZWFUZZTARGET_H_
#define TARGETS_SUPPORT_LZWFUZZTARGET_H_


#include "targets/FuzzTarget.h"


namespace freetype {


  class LzwFuzzTarget
    : public FuzzTarget
  {
  public:


    void
    run( const uint8_t*  data,
         size_t          size ) override;
  };
}


#endif // TARGETS_SUPPORT_LZWFUZZTARGET_H_
