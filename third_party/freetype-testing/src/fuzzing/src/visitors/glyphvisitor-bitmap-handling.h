// glyphvisitor-bitmap-handling.h
//
//   Invoke API functions that deal with bitmap handling.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef VISITORS_GLYPH_VISITOR_BITMAP_HANDLING_H_
#define VISITORS_GLYPH_VISITOR_BITMAP_HANDLING_H_


#include <ft2build.h>
#include FT_COLOR_H

#include "visitors/glyphvisitor.h"


namespace freetype {


  class GlyphVisitorBitmapHandling
    : public GlyphVisitor
  {
  public:


    void
    run( Unique_FT_Glyph  glyph )
    override;


  private:

    
    static const FT_Color   COLOUR_PINK;
    static const FT_Vector  SOURCE_OFFSET;


    bool
    extract_bitmap( Unique_FT_Glyph&  glyph,
                    FT_Bitmap&        bitmap );


    bool
    copy_bitmap( FT_Library        library,
                 const FT_Bitmap&  source,
                 FT_Bitmap&        target );


    void
    embolden( FT_Library  library,
              FT_Bitmap&  bitmap,
              FT_Pos      xStrength,
              FT_Pos      yStrength );


    void
    blend( FT_Library        library,
           const FT_Bitmap&  source,
           FT_Bitmap&        target );


    void
    convert( Unique_FT_Glyph&  glyph,
             FT_Bitmap&        bitmap,
             FT_Int            alignment );
  };
}


#endif // VISITORS_GLYPH_VISITOR_BITMAP_HANDLING_H_
