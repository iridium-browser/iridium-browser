// facevisitor-type1tables.h
//
//   Access Type 1 tables.  Note: for Multiple Master fonts, each instance has
//   its own dictionaries.
//
//   Drivers:
//     - CFF
//     - CID Type 1
//     - Type 1
//     - Type 42
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_FACE_VISITOR_TYPE_1_TABLES_H_
#define VISITORS_FACE_VISITOR_TYPE_1_TABLES_H_


#include <string>
#include <vector>

#include <ft2build.h>
#include FT_TYPE1_TABLES_H

#include "utils/logging.h"
#include "utils/utils.h"
#include "visitors/facevisitor.h"


namespace freetype {


  class FaceVisitorType1Tables
    : public FaceVisitor
  {
  public:


    void
    run( Unique_FT_Face  face )
    override;


  private:


    static const FT_Int   CHAR_STRING_INDEX_MAX       = 15;
    static const FT_Int   SUBR_INDEX_MAX              = 15;
    static const FT_Byte  BLUE_VALUE_INDEX_MAX        = 15;
    static const FT_Byte  OTHER_BLUE_INDEX_MAX        = 15;
    static const FT_Byte  FAMILY_BLUE_INDEX_MAX       = 15;
    static const FT_Byte  FAMILY_OTHER_BLUE_INDEX_MAX = 15;
    static const FT_Byte  STEM_SNAP_H_INDEX_MAX       = 15;
    static const FT_Byte  STEM_SNAP_V_INDEX_MAX       = 15;


    // @See: `FT_Get_PS_Font_Value()'.

    template<typename T>
    static void
    get_simple( Unique_FT_Face&  face,
                PS_Dict_Keys     key,
                T&               value )
    {
      (void) get_simple( face, key, 0, value );
    }
    

    // @See: `FT_Get_PS_Font_Value()'.

    template<typename T>
    static void
    get_simple( Unique_FT_Face&  face,
                PS_Dict_Keys     key,
                FT_UInt          index,
                T&               value )
    {
      FT_Long  size;


      size = FT_Get_PS_Font_Value( face.get(),
                                   key,
                                   index,
                                   &value,
                                   sizeof( value ) );

      if ( size > 0 )
        LOG( INFO ) << key << "[" << index << "]: " << value;
      else
      {
        LOG( INFO ) << key << "[" << index << "]: does not exist";
        value = static_cast<T>( 0 );
      }
    }


    // @See: `FT_Get_PS_Font_Value()'.

    static void
    get_string( Unique_FT_Face&          face,
                PS_Dict_Keys             key,
                std::vector<FT_String>&  str );


    // @See: `FT_Get_PS_Font_Value()'.

    static void
    get_string( Unique_FT_Face&          face,
                PS_Dict_Keys             key,
                FT_UInt                  index,
                std::vector<FT_String>&  str );


    // @Description:
    //   Loop over indices
    //     `0' to `min( max_value_computed, max_value_static ) - 1'
    //   and try to load every index of the given `key'.
    //
    // @Input:
    //   face ::
    //     The face that is to be tested.
    //
    //   max_value_computed ::
    //     The maximum value that has been computed by FreeType (given the
    //     current face).
    //
    //   max_value_static ::
    //     A static upper bound to avoid iterating thousand of times.
    //
    //   values_name ::
    //     A name that is to be used in `WARN_ABOUT_IGNORED_VALUES'.
    //
    //   key ::
    //     See: `FT_Get_PS_Font_Value'.
    //
    //   value ::
    //     See: `FT_Get_PS_Font_Value'.

    template<typename T>
    static void
    loop_simple( Unique_FT_Face&         face,
                 size_t                  max_value_computed,
                 size_t                  max_value_static,
                 std::string             values_name,
                 PS_Dict_Keys            key,
                 T&                      value )
    {
      for ( size_t  index = 0;
            index < max_value_computed &&
              index < max_value_static;
            index++ )
      {
        (void) get_simple( face, key, index, value );
      }

      WARN_ABOUT_IGNORED_VALUES( max_value_computed,
                                 max_value_static,
                                 values_name );
    }


    // @See: `FaceVisitorType1Tables::loop()'.

    static void
    loop_string( Unique_FT_Face&          face,
                 size_t                   max_value_computed,
                 size_t                   max_value_static,
                 std::string              values_name,
                 PS_Dict_Keys             key,
                 std::vector<FT_String>&  value );
  };
}

#endif // VISITORS_FACE_VISITOR_TYPE_1_TABLES_H_
