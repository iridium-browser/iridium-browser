// type1-tar.h
//
//   Fuzz Type 1 faces using tar sampling data.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_TYPE_1_TAR_H_
#define TARGETS_FONT_DRIVERS_TYPE_1_TAR_H_


#include "targets/font-drivers/type1.h"


namespace freetype {


  class Type1TarFuzzTarget
    : public Type1FuzzTarget
  {
  public:


    Type1TarFuzzTarget();
  };
}


#endif // TARGETS_FONT_DRIVERS_TYPE_1_TAR_H_
