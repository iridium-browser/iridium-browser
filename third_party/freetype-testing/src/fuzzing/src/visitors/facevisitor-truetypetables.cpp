// facevisitor-truetypetables.cpp
//
//   Implementation of FaceVisitorTrueTypeTables.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-truetypetables.h"

#include <cassert>

#include "utils/logging.h"

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H


  void
  freetype::FaceVisitorTrueTypeTables::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_Byte   buffer[100];
    FT_ULong  buffer_len = 100;

    FT_ULong  num_tables;
    FT_ULong  table_tag;
    FT_ULong  table_length;

    FT_ULong  cmap_language_id;
    FT_Long   cmap_format;


    assert( face != nullptr );

    for ( int i = FT_SFNT_HEAD; i < FT_SFNT_MAX; i++ )
    {
      LOG( INFO ) << "get sfnt table "
                  << ( i + 1 ) << "/" << FT_SFNT_MAX;
      
      (void) FT_Get_Sfnt_Table( face.get(),
                                static_cast<FT_Sfnt_Tag>( i ) );
    }

    LOG( INFO ) << "load whole font file";

    error = FT_Load_Sfnt_Table( face.get(), 0, 0, buffer, &buffer_len );
    LOG_FT_ERROR( "FT_Load_Sfnt_Table", error );

    error = FT_Sfnt_Table_Info ( face.get(), 0, nullptr, &num_tables );
    LOG_FT_ERROR( "FT_Sfnt_Table_Info", error );

    if ( error == 0 )
    {
      for ( FT_UInt table_index = 0;
            table_index < num_tables &&
              table_index < TABLE_INDEX_MAX;
            table_index++ )
      {
        error = FT_Sfnt_Table_Info( face.get(),
                                    table_index,
                                    &table_tag,
                                    &table_length );
        LOG_FT_ERROR( "FT_Sfnt_Table_Info", error );

        if ( error != 0 )
          continue;

        LOG( INFO ) << "load sfnt table "
                    << ( table_index + 1 ) << "/" << num_tables
                    << " (tag: " << table_tag << ")";

        error = FT_Load_Sfnt_Table( face.get(),
                                    table_tag,
                                    0,
                                    buffer,
                                    &buffer_len );
      }

      WARN_ABOUT_IGNORED_VALUES( num_tables, TABLE_INDEX_MAX, "tables" );
    }

    for ( FT_Int  charmap_index = 0;
          charmap_index < face->num_charmaps;
          charmap_index++ )
    {
      FT_CharMap  map = face->charmaps[charmap_index];

      
      cmap_language_id = FT_Get_CMap_Language_ID( map );
      cmap_format      = FT_Get_CMap_Format( map );

      LOG( INFO ) << "check charmap "
                  << ( charmap_index + 1 ) << "/"
                  << face->num_charmaps << ": "
                  << cmap_language_id << " (language id), "
                  << cmap_format << " (format)";
    }
  }
