// cidtype1-ftengine.h
//
//   Fuzz target for CID Type 1 fonts using FreeType's hinting engine.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_CID_TYPE_1_FT_ENGINE_H_
#define TARGETS_FONT_DRIVERS_CID_TYPE_1_FT_ENGINE_H_


#include "targets/font-drivers/cidtype1.h"


namespace freetype {


  class CidType1FtEngineFuzzTarget
    : public CidType1FuzzTarget
  {
  public:


    CidType1FtEngineFuzzTarget();
  };
}


#endif // TARGETS_FONT_DRIVERS_CID_TYPE_1_FT_ENGINE_H_
