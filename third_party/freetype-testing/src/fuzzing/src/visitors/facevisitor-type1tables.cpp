// facevisitor-type1tables.cpp
//
//   Implementation of FaceVisitorType1Tables.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-type1tables.h"

#include <cassert>
#include <vector>


  void
  freetype::FaceVisitorType1Tables::
  run( Unique_FT_Face  face )
  {
    FT_Error  error;

    FT_Int  has_glyph_names;

    PS_FontInfoRec  font_info_dict;
    PS_PrivateRec   private_dict;

    FT_Bool          bool_buffer;
    FT_Byte          byte_buffer;
    T1_EncodingType  encoding_type_buffer;
    FT_Fixed         fixed_buffer;
    FT_Int           int_buffer;
    FT_Long          long_buffer;
    FT_Short         short_buffer;
    FT_UShort        ushort_buffer;

    std::vector<FT_String>  string_buffer;

    FT_Int   num_char_strings;
    FT_Int   num_subrs = 0;
    FT_Byte  num_blue_values;
    FT_Byte  num_other_blues;
    FT_Byte  num_family_blues;
    FT_Byte  num_family_other_blues;
    FT_Byte  num_stem_snap_h;
    FT_Byte  num_stem_snap_v;
    

    assert( face != nullptr );

    has_glyph_names = FT_Has_PS_Glyph_Names( face.get() );

    LOG( INFO ) << "postscript glyph names seem to be "
                << ( has_glyph_names == 1 ? "reliable" : "unreliable" );

    // Note: we don't need to retrieve the glyph names here since
    // `FaceVisitorCharCodes' does that for a wide range of faces.  Any face
    // for which `FT_Has_PS_Glyph_Names' returns true will definitely be
    // checked by `FaceVisitorCharCodes'.

    error = FT_Get_PS_Font_Info( face.get(), &font_info_dict );
    
    if ( error == 0 )
      LOG( INFO ) << "retrieved font info dictionary";
    else if ( error == FT_Err_Invalid_Argument )
      LOG( INFO ) << "face is not postscript based";
    else
      LOG_FT_ERROR( "FT_Get_PS_Font_Info", error );

    error = FT_Get_PS_Font_Private( face.get(), &private_dict );
    
    if ( error == 0 )
      LOG( INFO ) << "retrieved private dictionary";
    else if ( error == FT_Err_Invalid_Argument )
      LOG( INFO ) << "face is not postscript based";
    else
      LOG_FT_ERROR( "FT_Get_PS_Font_Private", error );

    (void) get_simple( face, PS_DICT_FONT_TYPE,   byte_buffer   );
    (void) get_simple( face, PS_DICT_FONT_MATRIX, fixed_buffer  );
    (void) get_simple( face, PS_DICT_FONT_BBOX,   fixed_buffer  );
    (void) get_simple( face, PS_DICT_PAINT_TYPE,  byte_buffer   );
    (void) get_string( face, PS_DICT_FONT_NAME,   string_buffer );
    (void) get_simple( face, PS_DICT_UNIQUE_ID,   int_buffer    );

    (void) get_simple( face, PS_DICT_NUM_CHAR_STRINGS, num_char_strings );

    (void) loop_string( face,
                        num_char_strings,
                        CHAR_STRING_INDEX_MAX,
                        "char strings",
                        PS_DICT_CHAR_STRING_KEY,
                        string_buffer );

    (void) loop_string( face,
                        num_char_strings,
                        CHAR_STRING_INDEX_MAX,
                        "char strings",
                        PS_DICT_CHAR_STRING,
                        string_buffer );

    (void) get_simple( face, PS_DICT_ENCODING_TYPE,  encoding_type_buffer );
    (void) get_string( face, PS_DICT_ENCODING_ENTRY, string_buffer );
    (void) get_simple( face, PS_DICT_NUM_SUBRS,      int_buffer    );

    (void) loop_string( face,
                        num_subrs,
                        SUBR_INDEX_MAX,
                        "subrs",
                        PS_DICT_SUBR,
                        string_buffer );

    (void) get_simple( face, PS_DICT_STD_HW, ushort_buffer );
    (void) get_simple( face, PS_DICT_STD_VW, ushort_buffer );

    (void) get_simple( face, PS_DICT_NUM_BLUE_VALUES, num_blue_values);

    (void) loop_simple( face,
                        num_blue_values,
                        BLUE_VALUE_INDEX_MAX,
                        "blue values",
                        PS_DICT_BLUE_VALUE,
                        short_buffer );

    (void) loop_simple( face,
                        num_blue_values,
                        BLUE_VALUE_INDEX_MAX,
                        "blue values",
                        PS_DICT_BLUE_FUZZ,
                        int_buffer );

    (void) get_simple( face, PS_DICT_NUM_OTHER_BLUES, num_other_blues );

    (void) loop_simple( face,
                        num_other_blues,
                        OTHER_BLUE_INDEX_MAX,
                        "other blues",
                        PS_DICT_OTHER_BLUE,
                        short_buffer );

    (void) get_simple( face, PS_DICT_NUM_FAMILY_BLUES, num_family_blues );

    (void) loop_simple( face,
                        num_family_blues,
                        FAMILY_BLUE_INDEX_MAX,
                        "family blues",
                        PS_DICT_FAMILY_BLUE,
                        short_buffer );

    (void) get_simple( face,
                       PS_DICT_NUM_FAMILY_OTHER_BLUES,
                       num_family_other_blues );
    
    (void) loop_simple( face,
                        num_family_other_blues,
                        FAMILY_OTHER_BLUE_INDEX_MAX,
                        "family other blues",
                        PS_DICT_FAMILY_OTHER_BLUE,
                        short_buffer );

    (void) get_simple( face, PS_DICT_BLUE_SCALE, fixed_buffer );
    (void) get_simple( face, PS_DICT_BLUE_SHIFT, int_buffer );

    (void) get_simple( face, PS_DICT_NUM_STEM_SNAP_H, num_stem_snap_h );

    (void) loop_simple( face,
                        num_stem_snap_h,
                        STEM_SNAP_H_INDEX_MAX,
                        "stem snap hs",
                        PS_DICT_STEM_SNAP_H,
                        short_buffer );

    (void) get_simple( face, PS_DICT_NUM_STEM_SNAP_V, num_stem_snap_v );

    (void) loop_simple( face,
                        num_stem_snap_v,
                        STEM_SNAP_V_INDEX_MAX,
                        "stem snap vs",
                        PS_DICT_STEM_SNAP_V,
                        short_buffer );

    (void) get_simple( face, PS_DICT_FORCE_BOLD,     bool_buffer  );
    (void) get_simple( face, PS_DICT_RND_STEM_UP,    bool_buffer  );
    (void) get_simple( face, PS_DICT_MIN_FEATURE,    short_buffer );
    (void) get_simple( face, PS_DICT_LEN_IV,         int_buffer   );
    (void) get_simple( face, PS_DICT_PASSWORD,       long_buffer  );
    (void) get_simple( face, PS_DICT_LANGUAGE_GROUP, long_buffer  );

    (void) get_string( face, PS_DICT_VERSION,             string_buffer );
    (void) get_string( face, PS_DICT_NOTICE,              string_buffer );
    (void) get_string( face, PS_DICT_FULL_NAME,           string_buffer );
    (void) get_string( face, PS_DICT_FAMILY_NAME,         string_buffer );
    (void) get_string( face, PS_DICT_WEIGHT,              string_buffer );
    (void) get_simple( face, PS_DICT_IS_FIXED_PITCH,      bool_buffer   );
    (void) get_simple( face, PS_DICT_UNDERLINE_POSITION,  short_buffer  );
    (void) get_simple( face, PS_DICT_UNDERLINE_THICKNESS, ushort_buffer );
    (void) get_simple( face, PS_DICT_FS_TYPE,             ushort_buffer );
    (void) get_simple( face, PS_DICT_ITALIC_ANGLE,        long_buffer   );
  }


  void
  freetype::FaceVisitorType1Tables::
  get_string( Unique_FT_Face&          face,
              PS_Dict_Keys             key,
              std::vector<FT_String>&  str )
  {
    (void) get_string( face, key, 0, str );
  }


  void
  freetype::FaceVisitorType1Tables::
  get_string( Unique_FT_Face&          face,
              PS_Dict_Keys             key,
              FT_UInt                  index,
              std::vector<FT_String>&  str )
  {
    FT_Long  size;


    size = FT_Get_PS_Font_Value( face.get(),
                                 key,
                                 index,
                                 nullptr,
                                 0 );

    if ( size <= 0 )
    {
      LOG( INFO ) << key << "[" << index << "]: does not exist.";
      return;
    }

    // `size' is in bytes:
    str.resize( size / sizeof( FT_String ) );

    (void) FT_Get_PS_Font_Value( face.get(),
                                 key,
                                 index,
                                 str.data(),
                                 size );

    LOG( INFO ) << key << "[" << index << "]: '"
                << std::string( str.begin(), str.end() ) << "'";
  }


  void
  freetype::FaceVisitorType1Tables::
  loop_string( Unique_FT_Face&          face,
               size_t                   max_value_computed,
               size_t                   max_value_static,
               std::string              values_name,
               PS_Dict_Keys             key,
               std::vector<FT_String>&  value )
  {
    for ( size_t  index = 0;
          index < max_value_computed &&
            index < max_value_static;
          index++ )
    {
      (void) get_string( face, key, index, value );
    }

    WARN_ABOUT_IGNORED_VALUES( max_value_computed,
                               max_value_static,
                               values_name );
  }
