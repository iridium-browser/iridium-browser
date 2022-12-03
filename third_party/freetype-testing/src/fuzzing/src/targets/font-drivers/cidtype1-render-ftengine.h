// cidtype1-render-ftengine.h
//
//   Rendering CID Type 1 faces with FreeType's hinting engine.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_CID_TYPE_1_RENDER_FT_ENGINE_H_
#define TARGETS_FONT_DRIVERS_CID_TYPE_1_RENDER_FT_ENGINE_H_


#include "targets/font-drivers/cidtype1-render.h"


namespace freetype {


  class CidType1RenderFtEngineFuzzTarget
    : public CidType1RenderFuzzTarget
  {
  public:


    CidType1RenderFtEngineFuzzTarget();
  };
}


#endif // TARGETS_FONT_DRIVERS_CID_TYPE_1_RENDER_FT_ENGINE_H_
