// cidtype1-ftengine.cpp
//
//   Implementation of CidType1FtEngineFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/cidtype1-ftengine.h"


  freetype::CidType1FtEngineFuzzTarget::
  CidType1FtEngineFuzzTarget()
  {
    (void) set_property( "t1cid", "hinting-engine", &HINTING_FREETYPE );
  }
