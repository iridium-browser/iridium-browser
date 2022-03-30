// faceprepiterator-outlines.h
//
//   Iterator that prepares faces for outline usage.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef ITERATORS_FACE_PREP_ITERATOR_OUTLINES_H_
#define ITERATORS_FACE_PREP_ITERATOR_OUTLINES_H_


#include <memory> // std::unique_ptr
#include <tuple>
#include <vector>

#include "iterators/faceprepiterator.h"


namespace freetype {


  class FacePrepIteratorOutlines
    : public FacePrepIterator
  {
  public:


    typedef std::tuple<
      FT_UInt,                 // pixel width
      FT_UInt,                 // pixel height
      FT_F26Dot6,              // char width
      FT_F26Dot6,              // char height
      FT_UInt,                 // horz resolution
      FT_UInt>  CharSizeTuple; // vert resolution

    typedef std::vector<CharSizeTuple>  CharSizeTuples;


    FacePrepIteratorOutlines();


    void
    run( const std::unique_ptr<FaceLoader>&  face_loader )
    override;


  protected:


    virtual Unique_FT_Face
    get_prepared_face( const std::unique_ptr<FaceLoader>&  face_loader,
                       CharSizeTuples::size_type           index );


  private:


    CharSizeTuples  char_sizes;

      
    // @Description:
    //   Append a char size specification to `char_sizes'.
    //
    // @Input:
    //   pixel_width_ppem ::
    //     Use, as is.
    //
    //   pixel_height_ppem ::
    //     Use, as is.
    //
    //   char_width_pt ::
    //     Use, multiplied by 64 (convert pt to 1/64th of points).
    //
    //   char_height_pt ::
    //     Use, multiplied by 64 (convert pt to 1/64th of points).
    //
    //   horz_resolution_dpi ::
    //     Use, as is.
    //
    //   vert_resolution_dpi ::
    //     Use, as is.

    void
    append_char_size( FT_UInt     pixel_width_ppem,
                      FT_UInt     pixel_height_ppem,
                      FT_F26Dot6  char_width_pt,
                      FT_F26Dot6  char_height_pt,
                      FT_UInt     horz_resolution_dpi,
                      FT_UInt     vert_resolution_dpi );
  };
}


#endif // ITERATORS_FACE_PREP_ITERATOR_OUTLINES_H_
