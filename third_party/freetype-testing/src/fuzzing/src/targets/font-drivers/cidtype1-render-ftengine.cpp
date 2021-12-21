// cidtype1-render-ftengine.cpp
//
//   Implementation of CidType1RenderFtEngineFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/cidtype1-render-ftengine.h"


  freetype::CidType1RenderFtEngineFuzzTarget::
  CidType1RenderFtEngineFuzzTarget()
  {
    (void) set_property( "t1cid", "hinting-engine", &HINTING_FREETYPE );
  }
