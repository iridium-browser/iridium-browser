// facevisitor-charcodes.cpp
//
//   Implementation of FaceVisitorCharCodes.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-charcodes.h"

#include <cassert>

#include "utils/logging.h"


  freetype::FaceVisitorCharCodes::
  FaceVisitorCharCodes()
  {
    encodings = {
      std::make_pair( FT_ENCODING_NONE,           "none"           ),
      std::make_pair( FT_ENCODING_MS_SYMBOL,      "ms symbol"      ),
      std::make_pair( FT_ENCODING_UNICODE,        "unicode"        ),
      std::make_pair( FT_ENCODING_SJIS,           "sjis"           ),
      std::make_pair( FT_ENCODING_PRC,            "prc"            ),
      std::make_pair( FT_ENCODING_BIG5,           "big5"           ),
      std::make_pair( FT_ENCODING_WANSUNG,        "wansung"        ),
      std::make_pair( FT_ENCODING_JOHAB,          "johab"          ),
      std::make_pair( FT_ENCODING_ADOBE_STANDARD, "adobe standard" ),
      std::make_pair( FT_ENCODING_ADOBE_EXPERT,   "adobe expert"   ),
      std::make_pair( FT_ENCODING_ADOBE_CUSTOM,   "adobe custom"   ),
      std::make_pair( FT_ENCODING_ADOBE_LATIN_1,  "adobe latin 1"  ),
      std::make_pair( FT_ENCODING_OLD_LATIN_2,    "old latin 2"    ),
      std::make_pair( FT_ENCODING_APPLE_ROMAN,    "apple roman"    )
    };
  }
  

  void
  freetype::FaceVisitorCharCodes::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;
    FT_Int    num_charmaps;
    

    assert( face != nullptr );

    num_charmaps = face->num_charmaps;

    for ( auto  encoding : encodings )
    {
      error = FT_Select_Charmap( face.get(), encoding.first );

      // No need to log here since this function is expected to fail quite
      // often for arbitrary fonts.  In fact, it would be concerning if this
      // function does not fail at all for a specific font ;)

      if ( error != 0 )
        continue;

      LOG( INFO ) << "charmap exists for encoding: " << encoding.second;

      (void) slide_along( face );
    }

    for ( FT_Int  charmap_index = 0;
          charmap_index < num_charmaps &&
            charmap_index < CHARMAP_INDEX_MAX;
          charmap_index++ )
    {
      FT_CharMap charmap = face->charmaps[charmap_index];


      error = FT_Set_Charmap ( face.get(), charmap );
      LOG_FT_ERROR( "FT_Set_Charmap", error );

      if ( error != 0 )
        continue;

      LOG( INFO ) << "load charmap "
                  << ( charmap_index + 1 ) << "/" << num_charmaps;

      if ( FT_Get_Charmap_Index( charmap ) != charmap_index )
        LOG( ERROR ) << "FT_Get_Charmap_Index failed";

      (void) slide_along( face );
    }

    WARN_ABOUT_IGNORED_VALUES( num_charmaps,
                               CHARMAP_INDEX_MAX,
                               "character maps" );
  }


  void
  freetype::FaceVisitorCharCodes::
  slide_along( const Unique_FT_Face&  face )
  {
    FT_Error  error;

    FT_ULong  char_code;
    FT_UInt   glyph_index;

    FT_String  glyph_name[100];
    FT_UInt    glyph_name_length = 100;

    FT_UInt  slide_index = 0;


    assert( face != nullptr );

    char_code = FT_Get_First_Char( face.get(), &glyph_index );

    while ( glyph_index != 0 )
    {
      if ( slide_index++ >= SLIDE_ALONG_MAX )
      {
        LOG( WARNING ) << "aborted early: more char codes available";
        break;
      }

      if ( glyph_index != FT_Get_Char_Index ( face.get(), char_code ) )
        LOG( ERROR ) << "FT_Get_Char_Index failed";

      // More advanced logic with load flags happens in
      // `FaceVisitorLoadGlyphs*'.  Here, we mainly want to invoke
      // `FT_Load_Char' a few times.

      error = FT_Load_Char( face.get(), char_code, FT_LOAD_DEFAULT );
      LOG_FT_ERROR( "FT_Load_Char", error );

      if ( FT_HAS_GLYPH_NAMES( face.get() ) != 1 )
        LOG( INFO ) << "char code: " << char_code << ", "
                    << "glyph index: " << glyph_index;
      else
      {
        error = FT_Get_Glyph_Name( face.get(),
                                   glyph_index,
                                   (void*) glyph_name,
                                   glyph_name_length );
        LOG_FT_ERROR( "FT_Get_Glyph_Name", error );

        if ( error != 0 )
          continue;

        if ( glyph_index != FT_Get_Name_Index ( face.get(), glyph_name ) )
          LOG( ERROR ) << "FT_Get_Name_Index failed";
        else
          LOG( INFO ) << "char code: " << char_code << ", "
                      << "glyph index: " << glyph_index << ", "
                      << "name: " << glyph_name;
      }

      char_code = FT_Get_Next_Char( face.get(), char_code, &glyph_index );
    }
  }
