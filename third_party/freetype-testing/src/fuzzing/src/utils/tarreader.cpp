// tarreader.cpp
//
//   Implementation of TarReader.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "utils/tarreader.h"

#include <cstdlib>

#include <archive.h>
#include <archive_entry.h>

#include "utils/logging.h"


  bool
  freetype::TarReader::
  extract_data( const uint8_t*  data,
                size_t          size )
  {
    Unique_Archive  archive( archive_read_new(),
                             archive_read_free );

    struct archive_entry*  entry;


    if ( archive == nullptr )
    {
      LOG( ERROR ) << "archive_read_new failed\n";
      return false;
    }

    if ( archive_read_support_format_tar( archive.get() ) != ARCHIVE_OK )
    {
      LOG( ERROR ) << "archive_read_support_format_tar failed: "
                   << archive_error_string( archive.get() ) << "\n";
      return false;
    }

    if ( archive_read_open_memory(
           archive.get(),
           const_cast<void*>( reinterpret_cast<const void*>( data ) ),
           size ) != ARCHIVE_OK )
    {
      LOG( ERROR ) << "archive_read_open_memory failed: "
                   << archive_error_string( archive.get() ) << "\n";
      return false;
    }

    while ( archive_read_next_header( archive.get(),
                                      &entry ) == ARCHIVE_OK ) {

      int  r;

      const FT_Byte*  buffer;
      size_t          buffer_size;
      la_int64_t      offset;

      std::vector<FT_Byte>  entry_data;


      LOG( INFO ) << "extracting: "
                  << archive_entry_pathname( entry )
                  << "\n";

      for (;;)
      {
        r = archive_read_data_block( archive.get(),
                                     reinterpret_cast<const void**>( &buffer ),
                                     &buffer_size,
                                     &offset );

        if ( r == ARCHIVE_EOF )
          break;

        if ( r != ARCHIVE_OK )
        {
          LOG( ERROR ) << "archive_read_data_block failed: " << r << "\n";
          return false;
        }

        (void) entry_data.insert( entry_data.end(),
                                  buffer,
                                  buffer + buffer_size );
      }

      (void) files.emplace_back( entry_data );
    }

    return true;
  }
