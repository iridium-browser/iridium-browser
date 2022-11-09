// facevisitor-charcodes.h
//
//   Convert char codes to glyph indices and back.
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


#ifndef VISITORS_FACE_VISITOR_CHARCODES_H_
#define VISITORS_FACE_VISITOR_CHARCODES_H_


#include <string>
#include <utility> // std::pair
#include <vector>

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorCharCodes
    : public FaceVisitor
  {
  public:


    FaceVisitorCharCodes();


    void
    run( Unique_FT_Face  face )
    override;


  private:


    typedef std::vector<std::pair<FT_Encoding, std::string>>  Encodings;


    // These operations are cheap but no need to exaggerate:
    static const FT_UInt  SLIDE_ALONG_MAX   = 50;
    static const FT_Int   CHARMAP_INDEX_MAX = 10;

    Encodings  encodings;


    // @Description:
    //   Drive the `FT_Get_First_Char' and `FT_Get_Next_Char' train for a
    //   while ...
    //
    // @Input:
    //   face ::
    //     The face that will be used for sliding.

    void
    slide_along( const Unique_FT_Face&  face );
  };
}


#endif // VISITORS_FACE_VISITOR_CHARCODES_H_
