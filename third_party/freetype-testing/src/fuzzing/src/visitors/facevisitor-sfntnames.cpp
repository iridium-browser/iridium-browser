// facevisitor-sfntnames.cpp
//
//   Implementation of SfntNames.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-sfntnames.h"

#include <cassert>

#include "utils/logging.h"

#include <ft2build.h>
#include FT_SFNT_NAMES_H


  void
  freetype::FaceVisitorSfntNames::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_UInt         num_sfnt_names;
    FT_SfntName     sfnt_name;
    FT_SfntLangTag  sfnt_lang;


    assert( face != nullptr );

    num_sfnt_names = FT_Get_Sfnt_Name_Count( face.get() );

    for ( FT_UInt  index = 0;
          index < num_sfnt_names &&
            index < SFNT_NAME_MAX;
          index++ )
    {
      error = FT_Get_Sfnt_Name( face.get(), index, &sfnt_name );
      LOG_FT_ERROR( "FT_Get_Sfnt_Name", error );

      if ( error != 0 )
      {
        continue;
      }

      LOG_IF( INFO, error == 0 )
        << "sfnt name "
        << ( index + 1 ) << "/" << num_sfnt_names << ": "
        << sfnt_name.platform_id << "/"
        << sfnt_name.encoding_id << "/"
        << sfnt_name.language_id << "/"
        << sfnt_name.name_id << ": "
        << "'" << std::string( reinterpret_cast<char*>( sfnt_name.string ),
                               sfnt_name.string_len ) << "'";

      // [...] values equal or larger than 0x8000 [...] indicate a language
      // tag string [...]. Use [...] `FT_Get_Sfnt_LangTag' [...] to retrieve
      // the [...] language tag.

      if ( sfnt_name.language_id <= 0x8000U )
        continue;

      error = FT_Get_Sfnt_LangTag( face.get(),
                                   sfnt_name.language_id,
                                   &sfnt_lang );
      LOG_FT_ERROR( "FT_Get_Sfnt_LangTag", error );

      LOG_IF( INFO, error == 0 )
        << "sfnt lang tag "
        << ( index + 1 ) << "/" << num_sfnt_names << ": "
        << "'" << std::string( reinterpret_cast<char*>( sfnt_lang.string ),
                               sfnt_lang.string_len ) << "'";
    }

    WARN_ABOUT_IGNORED_VALUES( num_sfnt_names, SFNT_NAME_MAX, "names" );
  }
