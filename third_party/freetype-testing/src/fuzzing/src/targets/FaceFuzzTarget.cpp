// FaceFuzzTarget.cpp
//
//   Implementation of FaceFuzzTarget.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "targets/FaceFuzzTarget.h"

#include <cassert>

#include FT_AUTOHINTER_H
#include FT_MODULE_H

#include "utils/logging.h"


  const FT_UInt
  freetype::FaceFuzzTarget::
  HINTING_ADOBE = FT_HINTING_ADOBE;


  const FT_UInt
  freetype::FaceFuzzTarget::
  HINTING_FREETYPE = FT_HINTING_FREETYPE;


  void
  freetype::FaceFuzzTarget::
  run( const uint8_t*  data,
       size_t          size )
  {
    if ( size < 1 )
      return;

    assert( face_load_iterator != nullptr );

    (void) face_load_iterator->set_library( get_ft_library() );
    (void) face_load_iterator->set_raw_bytes( data, size );
    (void) face_load_iterator->run();
  }


  void
  freetype::FaceFuzzTarget::
  set_supported_font_format( FaceLoader::FontFormat  format )
  {
    assert( face_load_iterator != nullptr );
    (void) face_load_iterator->set_supported_font_format( format );
  }


#ifdef HAVE_ARCHIVE
  void
  freetype::FaceFuzzTarget::
  set_data_is_tar_archive( bool  is_tar_archive )
  {
    assert( face_load_iterator != nullptr );
    (void) face_load_iterator->set_data_is_tar_archive( is_tar_archive );
  }
#endif


  bool
  freetype::FaceFuzzTarget::
  set_property( const std::string  module_name,
                const std::string  property_name,
                const void*        value )
  {
    FT_Error  error;


    LOG( INFO ) << "setting '" << property_name
                << "' in '"   << module_name << "'";

    error = FT_Property_Set( get_ft_library(),
                             module_name.c_str(),
                             property_name.c_str(),
                             value );

    LOG_FT_ERROR( "FT_Property_Set", error );

    return error == 0 ? true : false;
  }


  std::unique_ptr<freetype::FaceLoadIterator>&
  freetype::FaceFuzzTarget::
  set_iterator( std::unique_ptr<FaceLoadIterator>  iterator )
  {
    face_load_iterator = std::move( iterator );
    return face_load_iterator;
  }
