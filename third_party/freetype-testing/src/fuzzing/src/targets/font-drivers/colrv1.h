// colrv1.h
//
//   Fuzz ColrV1 color font faces.
//
// Copyright 2021 by
// Dominik Röttsches
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FONT_DRIVERS_COLRV1_H_
#define TARGETS_FONT_DRIVERS_COLRV1_H_


#include "targets/FaceFuzzTarget.h"


namespace freetype {


  class ColrV1FuzzTarget
    : public FaceFuzzTarget
  {
  public:


    ColrV1FuzzTarget();
  };
}


#endif // TARGETS_FONT_DRIVERS_COLRV1_H_
