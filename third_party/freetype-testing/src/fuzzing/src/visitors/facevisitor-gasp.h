// facevisitor-gasp.h
//
//   Question the gasp table.
//
//   Drivers:
//     - CFF
//     - TrueType
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_GASP_H_
#define VISITORS_FACE_VISITOR_GASP_H_


#include <vector>

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorGasp
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    // some arbitrary seeds:
    std::vector<FT_UInt>  ppems{ 8, 16, 32, 64 };
  };
}


#endif // VISITORS_FACE_VISITOR_GASP_H_
