// facevisitor-colrv1.h
//
//   Access and traverse COLRv1 glyph formation.
//
// Copyright 2018-2021 by
// Armin Hasitzka, Dominik Röttsches.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_COLRV1_H_
#define VISITORS_FACE_VISITOR_COLRV1_H_


#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorColrV1
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    unsigned int num_traversed_glyphs = 0;
  };
}


#endif // VISITORS_FACE_VISITOR_COLRV1_H_
