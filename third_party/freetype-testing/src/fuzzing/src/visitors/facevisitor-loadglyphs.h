// facevisitor-loadglyphs.h
//
//   Load a bunch of glyphs with a variety of different flags.
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


#ifndef VISITORS_FACE_VISITOR_LOAD_GLYPHS_H_
#define VISITORS_FACE_VISITOR_LOAD_GLYPHS_H_


#include <utility> // std::pair
#include <vector>

#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorLoadGlyphs
    : public FaceVisitor
  {
  public:


    // @See: `FaceVisitorLoadGlyphs::set_num_used_glyphs()'.

    FaceVisitorLoadGlyphs( FT_Long  num_used_glyphs );


    void
    run( Unique_FT_Face  face )
    override;


  protected:


    // @Description:
    //   Define the amount of glyphs that this visitor uses;
    //   [0 .. `num_used_glyphs'].  This function HAS to be called at least
    //   once per visitor as `run()' would fail otherwise.
    //
    // @Input:
    //   glyphs ::
    //     Even though this is `FT_Long', this function really only accepts
    //     values that are `> 0'.

    void
    set_num_used_glyphs( FT_Long  glyphs );


    // @Description:
    //   Add a transformation that will be used with all load flags.
    //   Note:  the "empty" transformation (no transformation at all) will
    //   always be added automatically.
    //
    // @Input:
    //   matrix ::
    //     See `FT_Set_Transform'.
    //
    //   vector ::
    //     See `FT_Set_Transform'.

    void
    add_transformation( FT_Matrix*  matrix,
                        FT_Vector*  delta );


    // @Description:
    //   Set load flags that will be used in `run()'.
    //
    // @Input:
    //   load_flags ::
    //     A set of load flags.

    void
    add_load_flags( FT_Int32  load_flags );


  private:


    typedef std::vector<std::pair<FT_Matrix*, FT_Vector*>>  Transformations;


    FT_Long  num_used_glyphs = -1;

    Transformations        transformations;
    std::vector<FT_Int32>  load_flags;
  };
}


#endif // VISITORS_FACE_VISITOR_LOAD_GLYPHS_H_
