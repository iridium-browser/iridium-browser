// faceprepiterator-multiplemasters.h
//
//   Iterator that prepares faces for outline usage with multiple masters.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_FACE_PREP_ITERATOR_MULTIPLE_MASTERS_H_
#define ITERATORS_FACE_PREP_ITERATOR_MULTIPLE_MASTERS_H_


#include <ft2build.h>
#include FT_MULTIPLE_MASTERS_H

#include "iterators/faceprepiterator-outlines.h"


namespace freetype {


  class FacePrepIteratorMultipleMasters
    : public FacePrepIteratorOutlines
  {
  protected:


    Unique_FT_Face
    get_prepared_face( const std::unique_ptr<FaceLoader>&  face_loader,
                       CharSizeTuples::size_type           index )
    override;


  private:


    static const FT_UInt  AXIS_INDEX_MAX = 10;


    Unique_FT_Face
    free_and_return( FT_Library      library,
                     FT_MM_Var*      master,
                     Unique_FT_Face  face );
  };
}


#endif // ITERATORS_FACE_PREP_ITERATOR_MULTIPLE_MASTERS_H_
