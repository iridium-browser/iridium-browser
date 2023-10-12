// faceloaditerator.cpp
//
//   Implementation of FaceLoadIterator.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "iterators/faceloaditerator.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::FaceLoadIterator::
  set_supported_font_format( FaceLoader::FontFormat  format )
  {
    assert( face_loader != nullptr );
    (void) face_loader->set_supported_font_format( format );
  }


  void
  freetype::FaceLoadIterator::
  set_library( FT_Library  library )
  {
    assert( face_loader != nullptr );
    (void) face_loader->set_library( library );
  }


  void
  freetype::FaceLoadIterator::
  set_raw_bytes( const uint8_t*  data,
                 size_t          size )
  {
    assert( face_loader != nullptr );
    (void) face_loader->set_raw_bytes( data, size );
  }


#ifdef HAVE_ARCHIVE
  void
  freetype::FaceLoadIterator::
  set_data_is_tar_archive( bool  is_tar_archive )
  {
    assert( face_loader != nullptr );
    (void) face_loader->set_data_is_tar_archive( is_tar_archive );
  }
#endif


  void
  freetype::FaceLoadIterator::
  add_once_visitor( std::unique_ptr<FaceVisitor>  visitor )
  {
    (void) once_face_visitors.emplace_back( std::move( visitor ) );
  }


  void
  freetype::FaceLoadIterator::
  add_always_visitor( std::unique_ptr<FaceVisitor>  visitor )
  {
    (void) always_face_visitors.emplace_back( std::move( visitor ) );
  }


  void
  freetype::FaceLoadIterator::
  add_iterator( std::unique_ptr<FacePrepIterator>  iterator )
  {
    (void) face_prep_iterators.emplace_back( std::move( iterator ) );
  }


  void
  freetype::FaceLoadIterator::
  run()
  {
    Unique_FT_Face  face = make_unique_face();

    const char*  face_name;

    FT_Long  num_faces;
    FT_Long  num_instances;

    
    assert( face_loader != nullptr );

    num_faces = face_loader->get_num_faces();

    for ( auto  face_index = 0;
          face_index < num_faces &&
            face_index < FACE_INDEX_MAX;
          face_index++ )
    {
      (void) face_loader->set_face_index( face_index );

      if ( face_loader->load() == nullptr )
      {
        LOG( ERROR ) << "face_loader->load failed";
        continue;
      }

      if ( face_index == 0 )
      {
        LOG( INFO ) << "fs type flags: 0x" << std::hex
                    << FT_Get_FSType_Flags( face.get() );

        for ( auto&  visitor : once_face_visitors )
          visitor->run( face_loader->load() );
      }

      num_instances = face_loader->get_num_instances();

      for ( auto  instance_index = 0;
            instance_index < num_instances &&
              instance_index < INSTANCE_INDEX_MAX;
            instance_index++ )
      {
        face_loader->set_instance_index( instance_index );

        LOG( INFO ) << "using face "
                    << ( face_index + 1 ) << "/" << num_faces << ", "
                    << "instance "
                    << ( instance_index + 1 ) << "/" << num_instances;

        face = face_loader->load();

        if ( face == nullptr )
        {
          LOG( ERROR ) << "face_loader->load failed";
          continue;
        }

        face_name = FT_Get_Postscript_Name( face.get() );
        LOG_IF( INFO, face_name != nullptr )
          << "postscript name: " << face_name;

        for ( auto&  visitor : always_face_visitors )
          visitor->run( face_loader->load() );

        for ( auto&  iterator : face_prep_iterators )
          iterator->run( face_loader );
      }

      WARN_ABOUT_IGNORED_VALUES( num_instances,
                                 INSTANCE_INDEX_MAX,
                                 "instances" );
    }

    WARN_ABOUT_IGNORED_VALUES( num_faces, FACE_INDEX_MAX, "faces" );
  }
