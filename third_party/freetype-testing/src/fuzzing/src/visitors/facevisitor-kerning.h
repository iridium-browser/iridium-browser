// facevisitor-kerning.h
//
//   Check the kerning intensively.
//
//   Drivers:
//     - CFF
//     - PFR
//     - TrueType
//     - Type 1
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_KERNING_H_
#define VISITORS_FACE_VISITOR_KERNING_H_


#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorKerning
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    // FT_Get_Kerning runs 1134x at most:
    // \sum_{n = 2}^{19}(n) * 2 * 3
    //   - 2 directions: left -> right, right -> left
    //   - 3 kerning modes
    static const FT_Long  MAX_GLYPH_INDEX = 20;
  };
}


#endif // VISITORS_FACE_VISITOR_KERNING_H_
