// facevisitor-variants.cpp
//
//   Implementation of FaceVisitorVariants.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-variants.h"

#include <cassert>
#include <set>
#include <vector>

#include "utils/logging.h"


  void
  freetype::FaceVisitorVariants::
  run( Unique_FT_Face  face )
  {
    FT_UInt32*              raw_selectors;
    std::vector<FT_UInt32>  selectors;

    FT_UInt32*           raw_chars;
    std::set<FT_UInt32>  local_charcodes;
    std::set<FT_UInt32>  global_charcodes;

    FT_Int   is_default;
    FT_UInt  glyph_index;


    assert( face != nullptr );

    raw_selectors = FT_Face_GetVariantSelectors( face.get() );

    if ( raw_selectors == nullptr )
    {
      LOG( INFO ) << "no valid variation selector cmap subtable";
      return;
    }

    while ( *raw_selectors != 0 )
    {
      selectors.push_back( *raw_selectors++ );
      if ( selectors.size() >= VARIANT_SELECTORS_MAX )
        break;
    }

    for ( FT_UInt32  selector : selectors )
    {
      raw_chars = FT_Face_GetCharsOfVariant( face.get(), selector );
      
      if ( raw_chars == nullptr ) {
        LOG( INFO ) << "no valid cmap or invalid variation selector";
        continue;
      }
      
      local_charcodes.clear();
      while ( *raw_chars != 0 )
      {
        local_charcodes.insert(  *raw_chars );
        global_charcodes.insert( *raw_chars++ );
        if ( local_charcodes.size() >= LOCAL_CHARCODES_MAX )
          break;
      }

      for ( auto  charcode : local_charcodes )
      {
        is_default = FT_Face_GetCharVariantIsDefault( face.get(),
                                                      charcode,
                                                      selector );
        LOG( INFO ) << "variant "
                    << std::hex << "0x" << charcode << "/"
                    << std::dec << selector
                    << " is default: " << is_default;

        glyph_index = FT_Face_GetCharVariantIndex( face.get(),
                                                   charcode,
                                                   selector );

        LOG( INFO ) << "glpyh index of variant "
                    << std::hex << "0x" << charcode << "/"
                    << std::dec << selector << ": "
                    << glyph_index;
      }
    }

    // The return value is not overly interesting in this setting.  Unittests
    // should check if this function works in tandem with
    // `FT_Face_GetCharsOfVariant' but this is not in the scope of fuzzers.

    for ( auto  charcode : global_charcodes )
      (void) FT_Face_GetVariantsOfChar( face.get(), charcode );
  }
