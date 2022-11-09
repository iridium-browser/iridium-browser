// cff-ftengine.cpp
//
//   Implementation of CffFtEngineFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/cff-ftengine.h"


  freetype::CffFtEngineFuzzTarget::
  CffFtEngineFuzzTarget()
  {
    (void) set_property( "cff", "hinting-engine", &HINTING_FREETYPE );
  }
