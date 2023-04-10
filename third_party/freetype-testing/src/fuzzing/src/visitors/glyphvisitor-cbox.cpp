// glyphvisitor-cbox.cpp
//
//   Implementation of GlyphVisitorCBox.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/glyphvisitor-cbox.h"

#include <cassert>

#include "utils/logging.h"


  void
  freetype::GlyphVisitorCBox::
  run( Unique_FT_Glyph  glyph )
  {
    assert( glyph != nullptr );

    (void) query_cbox( glyph, FT_GLYPH_BBOX_UNSCALED,  "unscaled"  );
    (void) query_cbox( glyph, FT_GLYPH_BBOX_SUBPIXELS, "subpixels" );
    (void) query_cbox( glyph, FT_GLYPH_BBOX_GRIDFIT,   "gridfit"   );
    (void) query_cbox( glyph, FT_GLYPH_BBOX_TRUNCATE,  "truncate"  );
    (void) query_cbox( glyph, FT_GLYPH_BBOX_PIXELS,    "pixels"    );
  }


  void
  freetype::GlyphVisitorCBox::
  query_cbox( const Unique_FT_Glyph&  glyph,
              FT_Glyph_BBox_Mode      bbox_mode,
              const std::string&      name )
  {
    FT_BBox   acbox;


    (void) FT_Glyph_Get_CBox( glyph.get(), bbox_mode, &acbox );

    LOG( INFO ) << "cbox (" << name << "): "
                << "(" << acbox.xMin << ", " << acbox.yMin << ") / "
                << "(" << acbox.xMax << ", " << acbox.yMax << ")";
  }
