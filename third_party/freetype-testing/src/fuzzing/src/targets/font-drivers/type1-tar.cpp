// type1-tar.cpp
//
//   Implementation of Type1TarFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/font-drivers/type1-tar.h"


  freetype::Type1TarFuzzTarget::
  Type1TarFuzzTarget()
  {
    (void) set_data_is_tar_archive( true );
  }
