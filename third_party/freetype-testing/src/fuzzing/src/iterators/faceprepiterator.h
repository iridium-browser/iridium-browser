// faceprepiterator.h
//
//   Base class of iterators that prepare faces.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_FACE_PREP_ITERATOR_H_
#define ITERATORS_FACE_PREP_ITERATOR_H_


#include <memory> // std::unique_ptr

#include "iterators/glyphloaditerator.h"
#include "utils/faceloader.h"
#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FacePrepIterator
    : private noncopyable
  {
  public:


    virtual
    ~FacePrepIterator() = default;


    // @Description:
    //   Add a face visitor that is called with every prepared face.
    //
    // @Input:
    //   visitor ::
    //     A face visitor.
    //
    // @Return:
    //   A reference to the added visitor.

    void
    add_visitor( std::unique_ptr<FaceVisitor>  visitor );


    // @Description:
    //   Add a glyph load iterator that is called with every prepared face.
    //
    // @Input:
    //   iterator ::
    //     A face preparation iterator.
    //
    // @Return:
    //   A reference to the added iterator.

    void
    add_iterator( std::unique_ptr<GlyphLoadIterator>  iterator );


    virtual void
    run( const std::unique_ptr<FaceLoader>&  face_loader ) = 0;


  protected:


    std::vector<std::unique_ptr<FaceVisitor>>        face_visitors;
    std::vector<std::unique_ptr<GlyphLoadIterator>>  glyph_load_iterators;
  };
}


#endif // ITERATORS_FACE_PREP_ITERATOR_H_
