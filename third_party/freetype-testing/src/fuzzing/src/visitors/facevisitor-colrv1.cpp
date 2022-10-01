// facevisitor-colrv1.cpp
//
//   Finds and traverses COLRv1 glyphs using FreeType's
//   COLRv1 API.
//
// Copyright 2021 by
// Dominik Röttsches.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "visitors/facevisitor-colrv1.h"

#include <cassert>
#include <unordered_set>

#include "utils/logging.h"

#include <freetype/freetype.h>
#include <freetype/ftcolor.h>


template<> struct std::hash<FT_OpaquePaint>
{


  std::size_t
  operator()( const FT_OpaquePaint&  p ) const
  {
    std::size_t h1 = std::hash<FT_Byte*>{}( p.p );
    std::size_t h2 = std::hash<FT_Bool>{}( p.insert_root_transform );


    return h1 ^ (h2 << 1);
  }
};


template<> struct std::equal_to<FT_OpaquePaint>
{


  bool
  operator()( const FT_OpaquePaint&  lhs,
              const FT_OpaquePaint&  rhs ) const
  {
    return lhs.p == rhs.p &&
           lhs.insert_root_transform == rhs.insert_root_transform;
  }
};


namespace {


  using VisitedSet = std::unordered_set<FT_OpaquePaint>;


  constexpr unsigned long  MAX_TRAVERSE_GLYPHS = 5;


  class ScopedSetInserter
    : private freetype::noncopyable
  {
  public:


    ScopedSetInserter( VisitedSet&     set,
                       FT_OpaquePaint  paint)
      : visited_set( set ),
        p( paint )
    {


      visited_set.insert(p);
    }


    ~ScopedSetInserter()
    {
      visited_set.erase(p);
    }


  private:


    VisitedSet&     visited_set;
    FT_OpaquePaint  p;
  };


  bool colrv1_start_glyph( const FT_Face&           ft_face,
                           uint16_t                 glyph_id,
                           FT_Color_Root_Transform  root_transform,
                           VisitedSet&              visited_set );


  void iterate_color_stops ( FT_Face                face,
                             FT_ColorStopIterator*  color_stop_iterator )
  {
    const FT_UInt  num_color_stops = color_stop_iterator->num_color_stops;
    FT_ColorStop   color_stop;
    long           color_stop_index = 0;


    while ( FT_Get_Colorline_Stops( face,
                                    &color_stop,
                                    color_stop_iterator ) )
    {
      LOG( INFO ) << "Color stop " << color_stop_index
                  << "/" << num_color_stops - 1
                  << " stop offset: " << color_stop.stop_offset
                  << " palette index: " << color_stop.color.palette_index
                  << " alpha: " << color_stop.color.alpha;
    }
  }


  void colrv1_draw_paint( FT_Face        face,
                          FT_COLR_Paint  colrv1_paint )
  {
    switch ( colrv1_paint.format )
    {

    case FT_COLR_PAINTFORMAT_GLYPH:
    {
      LOG( INFO ) << "PaintGlyph";
      break;
    }

    case FT_COLR_PAINTFORMAT_SOLID:
    {
      LOG( INFO ) << "PaintSolid,"
                  << " palette_index: "
                  << colrv1_paint.u.solid.color.palette_index
                  << " alpha: " << colrv1_paint.u.solid.color.alpha;
      break;
    }

    case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT:
    {
      LOG( INFO ) << "PaintLinearGradient,"
                  << " p0.x " << colrv1_paint.u.linear_gradient.p0.x
                  << " p0.y " << colrv1_paint.u.linear_gradient.p0.y
                  << " p1.x " << colrv1_paint.u.linear_gradient.p1.x
                  << " p1.y " << colrv1_paint.u.linear_gradient.p1.y
                  << " p2.x " << colrv1_paint.u.linear_gradient.p2.x
                  << " p2.y " << colrv1_paint.u.linear_gradient.p2.y;

      iterate_color_stops(
        face,
        &colrv1_paint.u.linear_gradient.colorline.color_stop_iterator );

      break;
    }

    case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT:
    {
      LOG( INFO ) << "PaintRadialGradient,"
                  << " c0.x " << colrv1_paint.u.radial_gradient.c0.x
                  << " c0.y " << colrv1_paint.u.radial_gradient.c0.y
                  << " c1.x " << colrv1_paint.u.radial_gradient.c1.x
                  << " c1.y " << colrv1_paint.u.radial_gradient.c1.y
                  << " r0 " << colrv1_paint.u.radial_gradient.r0
                  << " r1 " << colrv1_paint.u.radial_gradient.r1;

      iterate_color_stops(
        face,
        &colrv1_paint.u.radial_gradient.colorline.color_stop_iterator );

      break;
    }

    case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT:
    {
      LOG( INFO ) << "PaintSweepGradient,"
                  << " center.x " << colrv1_paint.u.sweep_gradient.center.x
                  << " center.y " << colrv1_paint.u.sweep_gradient.center.y
                  << " start_angle "
                  << colrv1_paint.u.sweep_gradient.start_angle
                  << " end_angle " << colrv1_paint.u.sweep_gradient.end_angle;

      iterate_color_stops(
        face,
        &colrv1_paint.u.sweep_gradient.colorline.color_stop_iterator );

      break;
    }

    case FT_COLR_PAINTFORMAT_TRANSFORM:
    {
      LOG( INFO ) << "PaintTransformed,"
                  << " xx " << colrv1_paint.u.transform.affine.xx
                  << " xy " << colrv1_paint.u.transform.affine.xy
                  << " yx " << colrv1_paint.u.transform.affine.yx
                  << " yy " << colrv1_paint.u.transform.affine.yy
                  << " dx " << colrv1_paint.u.transform.affine.dx
                  << " dy " << colrv1_paint.u.transform.affine.dy;
      break;
    }

    case FT_COLR_PAINTFORMAT_TRANSLATE:
    {
      LOG( INFO ) << "PaintTranslate,"
                  << " dx " << colrv1_paint.u.translate.dx
                  << " dy " << colrv1_paint.u.translate.dy;
      break;
    }

    case FT_COLR_PAINTFORMAT_SCALE:
    {
      LOG( INFO ) << "PaintScale,"
                  << " center.x " << colrv1_paint.u.scale.center_x
                  << " center.y " << colrv1_paint.u.scale.center_y
                  << " scale.x " << colrv1_paint.u.scale.scale_x
                  << " scale.y " << colrv1_paint.u.scale.scale_y;
      break;
    }

    case FT_COLR_PAINTFORMAT_ROTATE:
    {
      LOG( INFO ) << "PaintRotate,"
                  << " center.x " << colrv1_paint.u.rotate.center_x
                  << " center.y " << colrv1_paint.u.rotate.center_y
                  << " angle " << colrv1_paint.u.rotate.angle;
      break;
    }

    case FT_COLR_PAINTFORMAT_SKEW:
    {
      LOG( INFO ) << "PaintSkew,"
                  << " center.x " << colrv1_paint.u.skew.center_x
                  << " center.y " << colrv1_paint.u.skew.center_y
                  << " x_skew_angle " << colrv1_paint.u.skew.x_skew_angle
                  << " y_skew_angle " << colrv1_paint.u.skew.y_skew_angle;
      break;
    }

    case FT_COLR_PAINTFORMAT_COLR_GLYPH:
    case FT_COLR_PAINTFORMAT_COLR_LAYERS:
    case FT_COLR_PAINTFORMAT_COMPOSITE:
    case FT_COLR_PAINT_FORMAT_MAX:
    case FT_COLR_PAINTFORMAT_UNSUPPORTED:
    {
        // No action for these paint format types in drawing.
        break;
    }
    }
  }


  bool colrv1_traverse_paint( FT_Face         face,
                              FT_OpaquePaint  opaque_paint,
                              VisitedSet&     visited_set )
  {
    FT_COLR_Paint  paint;

    bool  traverse_result = true;


    if ( visited_set.find( opaque_paint ) != visited_set.end() )
    {
      LOG( ERROR ) << "Paint cycle detected, aborting.";
      return false;
    }


    ScopedSetInserter  scoped_set_inserter( visited_set, opaque_paint );


    if ( !FT_Get_Paint( face, opaque_paint, &paint ) )
    {
      LOG( ERROR ) << "FT_Get_Paint failed.";
      return false;
    }

    // Keep track of failures to retrieve the FT_COLR_Paint from FreeType in the
    // recursion, cancel recursion when a paint retrieval fails.
    switch ( paint.format )
    {

    case FT_COLR_PAINTFORMAT_COLR_LAYERS:
    {
      FT_LayerIterator&  layer_iterator = paint.u.colr_layers.layer_iterator;
      FT_OpaquePaint     opaque_paint_fetch;

      LOG( INFO ) << "PaintColrLayers,"
                  << " num_layers " << layer_iterator.num_layers;

      opaque_paint_fetch.p = nullptr;
      while ( FT_Get_Paint_Layers( face,
                                   &layer_iterator,
                                   &opaque_paint_fetch ) )
        colrv1_traverse_paint( face,
                               opaque_paint_fetch,
                               visited_set );

      break;
    }

    case FT_COLR_PAINTFORMAT_GLYPH:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.glyph.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_COLR_GLYPH:

      LOG( INFO ) << "PaintColrGlyph,"
                  << " glyphId " << paint.u.colr_glyph.glyphID;

      traverse_result = colrv1_start_glyph( face,
                                            paint.u.colr_glyph.glyphID,
                                            FT_COLOR_NO_ROOT_TRANSFORM,
                                            visited_set );
      break;

    case FT_COLR_PAINTFORMAT_TRANSFORM:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.transform.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_TRANSLATE:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.translate.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_SCALE:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.scale.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_ROTATE:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.rotate.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_SKEW:
      colrv1_draw_paint( face, paint );
      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.skew.paint,
                                               visited_set );
      break;

    case FT_COLR_PAINTFORMAT_COMPOSITE:
    {
      LOG( INFO ) << "PaintComposite,"
                  << " composite_mode " << paint.u.composite.composite_mode;

      traverse_result = colrv1_traverse_paint( face,
                                               paint.u.composite.backdrop_paint,
                                               visited_set );
      traverse_result =
        traverse_result && colrv1_traverse_paint( face,
                                                  paint.u.composite.source_paint,
                                                  visited_set );
        break;
    }

    case FT_COLR_PAINTFORMAT_SOLID:
    case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT:
    case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT:
    case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT:
    {
      colrv1_draw_paint( face, paint );
      break;
    }

    case FT_COLR_PAINT_FORMAT_MAX:
    case FT_COLR_PAINTFORMAT_UNSUPPORTED:
      LOG ( INFO ) << "Invalid paint format.";
      break;
    }

    return traverse_result;
  }


  bool colrv1_start_glyph( const FT_Face&           ft_face,
                           uint16_t                 glyph_id,
                           FT_Color_Root_Transform  root_transform,
                           VisitedSet&              visited_set )
  {
    FT_OpaquePaint  opaque_paint;
    bool            has_colrv1_layers = false;


    opaque_paint.p = nullptr;

    if ( FT_Get_Color_Glyph_Paint( ft_face,
                                   glyph_id,
                                   root_transform,
                                   &opaque_paint ) )
    {
      FT_ClipBox colr_glyph_clip_box;
      FT_Get_Color_Glyph_ClipBox( ft_face, glyph_id, &colr_glyph_clip_box );
      LOG( INFO ) << "ClipBox,"
                  << " glyph_id " << glyph_id
                  << " bottom_left ("
                  << colr_glyph_clip_box.bottom_left.x << ", "
                  << colr_glyph_clip_box.bottom_left.y << ")"
                  << " top_left ("
                  << colr_glyph_clip_box.top_left.x << ", "
                  << colr_glyph_clip_box.top_left.y << ")"
                  << " top_right ("
                  << colr_glyph_clip_box.top_right.x << ", "
                  << colr_glyph_clip_box.top_right.y << ")"
                  << " bottom_right ("
                  << colr_glyph_clip_box.bottom_right.x << ", "
                  << colr_glyph_clip_box.bottom_right.y << ")";

      has_colrv1_layers = true;
      colrv1_traverse_paint( ft_face, opaque_paint, visited_set );
    }

    return has_colrv1_layers;
  }
}


  void
  freetype::FaceVisitorColrV1::
  run( Unique_FT_Face  face )
  {
    FT_OpaquePaint  opaque_paint;
    unsigned long   num_glyphs;


    assert( face != nullptr );

    num_glyphs = face->num_glyphs;

    LOG( INFO ) << "Starting COLR v1 traversal.";

    opaque_paint.p = nullptr;

    for ( uint16_t  glyph_id = 0;
          glyph_id < num_glyphs;
          glyph_id++ )
    {
      if ( num_traversed_glyphs >= MAX_TRAVERSE_GLYPHS )
      {
        LOG( INFO ) << "Finished with this font after "
                    << MAX_TRAVERSE_GLYPHS
                    << " traversed glyphs.";
        return;
      }

      VisitedSet visited_set;
      if ( !colrv1_start_glyph( face.get(),
                                glyph_id,
                                FT_COLOR_INCLUDE_ROOT_TRANSFORM,
                                visited_set ) )
      {
        LOG( INFO ) << "No COLRv1 glyph for glyph id " << glyph_id << ".";
        continue;
      }

      num_traversed_glyphs++;
    }
  }
