// truetype-render.h
//
//   Fuzz target for rendering TrueType faces.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_TRUETYPE_RENDER_H_
#define TARGETS_FONT_DRIVERS_TRUETYPE_RENDER_H_


#include <ft2build.h>
#include FT_DRIVER_H

#include "targets/FaceFuzzTarget.h"


namespace freetype {


  class TrueTypeRenderFuzzTarget
    : public FaceFuzzTarget
  {
  public:


    TrueTypeRenderFuzzTarget();


  protected:


    // We need variables holding the enum values to feed them to
    // `FT_Property_Set':

    static const FT_UInt  INTERPRETER_VERSION_35;
    static const FT_UInt  INTERPRETER_VERSION_38;
    static const FT_UInt  INTERPRETER_VERSION_40;
  };
}


#endif // TARGETS_FONT_DRIVERS_TRUETYPE_RENDER_H_
