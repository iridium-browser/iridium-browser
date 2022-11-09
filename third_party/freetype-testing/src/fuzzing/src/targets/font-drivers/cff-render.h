// cff-render.h
//
//   Render CFF faces.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_CFF_RENDER_H_
#define TARGETS_FONT_DRIVERS_CFF_RENDER_H_


#include "targets/FaceFuzzTarget.h"


namespace freetype {


  class CffRenderFuzzTarget
    : public FaceFuzzTarget
  {
  public:


    CffRenderFuzzTarget();
  };
}


#endif // TARGETS_FONT_DRIVERS_CFF_RENDER_H_
