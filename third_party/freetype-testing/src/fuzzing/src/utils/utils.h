// utils.h
//
//   Collection of globally useful snippets.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_


#define WARN_ABOUT_IGNORED_VALUES( available, capped, name )  \
  do                                                          \
  {                                                           \
    LOG_IF( WARNING, (available) > (capped) )                 \
      << "aborted early: "                                    \
      << ( (available) - (capped) )                           \
      << " " << name << " ignored";                           \
  } while (0)


#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


struct archive;


namespace freetype
{

  typedef std::unique_ptr<archive,
                          int( * )( archive* ) >  Unique_Archive;

  typedef std::unique_ptr<FT_FaceRec,
                          decltype( FT_Done_Face )*>  Unique_FT_Face;

  typedef std::unique_ptr<FT_GlyphRec,
                          decltype( FT_Done_Glyph )*>  Unique_FT_Glyph;


  // @Description:
  //   As of today (Summer 2018), `std::make_unique' is still not reliably
  //   available.  This version of `make_unique' helps to bridge the gap
  //   since the usefulness of `make_unique' is undisputed.
  //   Source: https://isocpp.org/files/papers/N3656.txt

  template<typename T, typename... Args>
  std::unique_ptr<T>
  make_unique( Args&&...  args )
  {
    return std::unique_ptr<T>( new T( std::forward<Args>( args )... ) );
  }


  Unique_FT_Face
  make_unique_face( FT_Face  face = nullptr );


  Unique_FT_Glyph
  make_unique_glyph( FT_Glyph  glyph = nullptr );


  // @Description:
  //   Creates a copy of a glyph.
  //
  // @Input:
  //   glyph ::
  //     A glyph that will be copied.
  //
  // @Return:
  //   A freshly created copy of the glyph.

  Unique_FT_Glyph
  copy_unique_glyph( const Unique_FT_Glyph&  glyph );


  // @Description:
  //   Extract a loaded glyph from the face via `FT_Get_Glyph'.  Mind: This
  //   can only be done ONCE per loaded face as the new glyph only
  //   references bitmaps and outlines of the face's glyph slot.  Copy the
  //   returned glyph immediately and avoid using it directly (unless in
  //   very special cases).
  //
  // @Input:
  //   face ::
  //     A face that has a loaded glyph in its glyph slot.
  //
  // @Return:
  //   The last loaded glyph of the input face or `nullptr' if any error
  //   occurred.

  Unique_FT_Glyph
  get_unique_glyph_from_face( const Unique_FT_Face&  face );


  // @See: `glyph_has_reasonable_size'.

  bool
  glyph_has_reasonable_size( const Unique_FT_Glyph&  glyph,
                             FT_Pos                  reasonable_pixels );


  // @Description:
  //   Check if the given glyph's bitmap exceeds certain sizes.
  //
  // @Input:
  //   glyph ::
  //     A glpyh that is to be evaluated.
  //
  //   reasonable_pixels ::
  //     Amount of total pixels.  `0' means:  accept anything.
  //
  //   reasonable_width ::
  //     Amount of vertical pixels.  `0' means:  accept anything.
  //
  //   reasonable_height ::
  //     Amount of horizontal pixels.  `0' means:  accept anything.
  //
  // @Return:
  //    `true', if the given glyph's bitmap has less than or equally many
  //    pixels as the respective reasonabale sizes.

  bool
  glyph_has_reasonable_size( const Unique_FT_Glyph&  glyph,
                             FT_Pos                  reasonable_pixels,
                             FT_Pos                  reasonable_width,
                             FT_Pos                  reasonable_height );


  // @Description:
  //   Check the size of the glyph and evaluate if it is worth working with
  //   it.  This function logs all relevant details like glyph size and
  //   success validity.
  //
  // @Input:
  //   glyph ::
  //     A glpyh that is to be evaluated.
  //
  // @Return:
  //   `true', if `fuzzing::get_glyph_pixels()' is below a reasonale
  //   threshold, `false' if it is above it.

  bool
  glyph_has_reasonable_work_size( const Unique_FT_Glyph&  glyph );


  // @Description:
  //   Check the size of the glyph and evaluate if it is worth rendering it
  //   This function logs all relevant details like glyph size and success
  //   validity.
  //
  // @Input:
  //   glyph ::
  //     A glpyh that is to be evaluated.
  //
  // @Return:
  //   `true', if `fuzzing::get_glyph_pixels()' is below a reasonale
  //   threshold, `false' if it is above it.

  bool
  glyph_has_reasonable_render_size( const Unique_FT_Glyph&  glyph );
}


#endif // UTILS_UTILS_H_
