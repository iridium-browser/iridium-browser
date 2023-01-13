// faceloader.cpp
//
//   Implementation of FaceLoader.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "utils/faceloader.h"

#include <cassert>

#include FT_FONT_FORMATS_H

#include "utils/logging.h"


  void
  freetype::FaceLoader::
  set_supported_font_format( FontFormat  format )
  {
    supported_font_format = format;
    
    switch ( format )
    {
    case FontFormat::NONE:        supported_font_format_string = "None";        break;
    case FontFormat::BDF:         supported_font_format_string = "BDF";         break;
    case FontFormat::CID_TYPE_1:  supported_font_format_string = "CID Type 1";  break;
    case FontFormat::CFF:         supported_font_format_string = "CFF";         break;
    case FontFormat::PCF:         supported_font_format_string = "PCF";         break;
    case FontFormat::PFR:         supported_font_format_string = "PFR";         break;
    case FontFormat::TRUETYPE:    supported_font_format_string = "TrueType";    break;
    case FontFormat::TYPE_1:      supported_font_format_string = "Type 1";      break;
    case FontFormat::TYPE_42:     supported_font_format_string = "Type 42";     break;
    case FontFormat::WINDOWS_FNT: supported_font_format_string = "Windows FNT"; break;
    }
  }


  void
  freetype::FaceLoader::
  set_library( FT_Library  library )
  {
    this->library = library;
  }


  void
  freetype::FaceLoader::
  set_raw_bytes( const uint8_t*  data,
                 size_t          size )
  {
    assert( size > 0 );

    (void) files.clear();

#ifdef HAVE_ARCHIVE
    if ( data_is_tar_archive == true )
      (void) tarreader.extract_data( data, size );
    else
#endif
      (void) files.emplace_back( data, data + size );

    num_faces     = -1;
    num_instances = -1;
  }

#ifdef HAVE_ARCHIVE
  void
  freetype::FaceLoader::
  set_data_is_tar_archive( bool  is_tar_archive )
  {
    data_is_tar_archive = is_tar_archive;
  }
#endif


  void
  freetype::FaceLoader::
  set_face_index( FT_Long  index )
  {
    if ( index != face_index )
      num_instances = -1;

    if ( index < 0 || index >= get_num_faces() )
    {
      LOG( ERROR ) << "invalid face index: " << index;
      face_index = 0;
    }
    else
      face_index = index;
  }

    
  void
  freetype::FaceLoader::
  set_instance_index( FT_Long  index )
  {
    if ( index < 0 || index >= get_num_instances() )
    {
      LOG( ERROR ) << "invalid instance index: " << index;
      instance_index = 0;
    }
    else
      instance_index = index;
  }


  FT_Long
  freetype::FaceLoader::
  get_num_faces()
  {
    if ( num_faces < 0 )
    {
      Unique_FT_Face  face = load_face();

      
      if ( face == nullptr )
      {
        LOG( ERROR ) << "load_face failed";
        num_faces = 0;
      }
      else
        num_faces = face->num_faces;
    }
    return num_faces;
  }


  FT_Long
  freetype::FaceLoader::
  get_num_instances()
  {
    if ( num_instances < 0 )
    {
      Unique_FT_Face  face = load_face( -1 * ( face_index + 1 ) );


      if ( face == nullptr )
      {
        LOG( ERROR ) << "load_face failed";
        num_instances = 0;
      }
      else
        num_instances = ( face->style_flags >> 16 ) + 1;
    }
    return num_instances;    
  }


  freetype::Unique_FT_Face
  freetype::FaceLoader::
  load()
  {
    return load_face( face_index, instance_index );
  }


  freetype::Unique_FT_Face
  freetype::FaceLoader::
  load_face( FT_Long  face_index,
             FT_Long  instance_index )
  {
    FT_Error  error;
    FT_Face   face;


    if ( files.size() < 1 )
    {
      LOG( ERROR ) << "missing data; no data to load a face from";
      return make_unique_face();
    }

    if ( face_index >= 0 )
      face_index += ( instance_index << 16 );

    error = FT_New_Memory_Face( library,
                                files[0].data(),
                                files[0].size(),
                                face_index,
                                &face );

    LOG_FT_ERROR( "FT_New_Memory_Face", error );

    if ( error != 0 )
      return make_unique_face();

    // If we have more than a single input file coming from an archive,
    // attach them (starting with the second file) using the order given
    // in the archive.
    
    for ( size_t  i = 1; i < files.size(); i++ )
    {
      FT_Error  error;

      FT_Open_Args  open_args =
      {
        .flags       = FT_OPEN_MEMORY,
        .memory_base = files[i].data(),
        .memory_size = static_cast<FT_Long>( files[i].size() )
      };


      error = FT_Attach_Stream( face, &open_args );
      LOG_FT_ERROR( "FT_Attach_Stream", error );

      if ( error != 0 )
      {
        (void) FT_Done_Face( face );
        return make_unique_face();
      }
    }

    std::string  font_format( FT_Get_Font_Format( face ) );
    if ( font_format != supported_font_format_string )
    {
      LOG( ERROR ) << "invalid font format: "
                   << "received '" << font_format << "' but "
                   << "expected '" << supported_font_format_string << "'";
      (void) FT_Done_Face( face );
      return make_unique_face();
    }

    return make_unique_face( face );
  }
