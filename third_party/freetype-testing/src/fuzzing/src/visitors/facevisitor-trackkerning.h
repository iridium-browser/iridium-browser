// facevisitor-trackkerning.h
//
//   Fuzz track kerning.
//
//   Drivers:
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


#ifndef VISITORS_FACE_VISITOR_TRACK_KERNING_H_
#define VISITORS_FACE_VISITOR_TRACK_KERNING_H_


#include <vector>

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorTrackKerning
    : public FaceVisitor
  {
  public:


    FaceVisitorTrackKerning();


    void
    run( Unique_FT_Face  face )
    override;


  private:


    std::vector<FT_Fixed>  point_sizes;
    std::vector<FT_Int>    degrees;
  };
}


#endif // VISITORS_FACE_VISITOR_TRACK_KERNING_H_
