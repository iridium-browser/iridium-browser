// facevisitor-variants.h
//
//   Play with unicode variation sequences.
//
//   Drivers: all
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_VARIANTS_H_
#define VISITORS_FACE_VISITOR_VARIANTS_H_


#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorVariants
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    static const size_t  VARIANT_SELECTORS_MAX =  10;
    static const size_t  LOCAL_CHARCODES_MAX   =  50;
  };
}


#endif // VISITORS_FACE_VISITOR_VARIANTS_H_
