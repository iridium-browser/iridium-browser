// FaceFuzzTarget.h
//
//   Base class of face fuzz targets.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef TARGETS_FACEFUZZTARGET_H_
#define TARGETS_FACEFUZZTARGET_H_


#include <functional>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "iterators/faceloaditerator.h"
#include "iterators/faceprepiterator.h"
#include "iterators/glyphloaditerator.h"
#include "targets/FuzzTarget.h"
#include "utils/faceloader.h"
#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceFuzzTarget
    : public FuzzTarget
  {
  public:


    // @Description:
    //   Run some input on the fuzz target.
    //
    // @Input:
    //   data ::
    //     Fuzzed input data.
    //
    //   size ::
    //     The size of fuzzed input data.

    void
    run( const uint8_t*  data,
         size_t          size ) override;


  protected:


    FaceFuzzTarget() = default;


    // @See: `FaceLoader::set_supported_font_format()'

    void
    set_supported_font_format( FaceLoader::FontFormat  format );


#ifdef HAVE_ARCHIVE
    // @See: `FaceLoader::set_data_is_tar_archive()'

    void
    set_data_is_tar_archive( bool  is_tar_archive );
#endif


    // @Description:
    //   Set a FreeType property via `FT_Property_Set'.
    //
    // @Input:
    //   module_name ::
    //     See `FT_Property_Set'.
    //
    //   property_name ::
    //     See `FT_Property_Set'.
    //
    //   value ::
    //     See `FT_Property_Set'.
    //
    // @Return:
    //   `false' if `FT_Property_Set' returns an error, 'true' otherwise.

    bool
    set_property( const std::string  module_name,
                  const std::string  property_name,
                  const void*        value );


    // @Description:
    //   Set the face load iterator for this fuzz target.  This function
    //   overwrites other iterators that have been set before.
    //
    // @Input:
    //   iterator ::
    //     An face load iterator that will be used by `run()'.
    //
    // @Return:
    //   A reference to the added iterator.

    std::unique_ptr<FaceLoadIterator>&
    set_iterator( std::unique_ptr<FaceLoadIterator>  iterator );


  protected:


    static const FT_UInt  HINTING_ADOBE;
    static const FT_UInt  HINTING_FREETYPE;


  private:


    std::unique_ptr<FaceLoadIterator>  face_load_iterator;
  };
}


#endif // TARGETS_FACEFUZZTARGET_H_
