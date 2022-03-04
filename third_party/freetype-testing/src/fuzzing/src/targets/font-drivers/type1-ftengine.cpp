// type1-ftengine.cpp
//
//   Implementation of Type1FtEngineFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/type1-ftengine.h"


  freetype::Type1FtEngineFuzzTarget::
  Type1FtEngineFuzzTarget()
  {
    (void) set_property( "type1", "hinting-engine", &HINTING_FREETYPE );
  }
