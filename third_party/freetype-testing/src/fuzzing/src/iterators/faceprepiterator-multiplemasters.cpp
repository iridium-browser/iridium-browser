// faceprepiterator-multiplemasters.cpp
//
//   Implementation of FacePrepIteratorMultipleMasters.
//
// Copyright 2018-2019 by
// Armin Hasitzka, David Turner, Robert Wilhelm, and Werner Lemberg.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include "iterators/faceprepiterator-multiplemasters.h"

#include "utils/logging.h"


  freetype::Unique_FT_Face
  freetype::FacePrepIteratorMultipleMasters::
  get_prepared_face( const std::unique_ptr<FaceLoader>&  face_loader,
                     CharSizeTuples::size_type           index )
  {
    FT_Error  error;

    Unique_FT_Face  face =
      FacePrepIteratorOutlines::get_prepared_face( face_loader, index );

    FT_Library             library;
    FT_MM_Var*             master = nullptr;
    std::vector<FT_Fixed>  coords;


    if ( face == nullptr )
      return make_unique_face();

    if ( FT_HAS_MULTIPLE_MASTERS ( face ) == 0 )
      return make_unique_face();

    library = face->glyph->library;

    error = FT_Get_MM_Var( face.get(), &master );
    LOG_FT_ERROR( "FT_Get_MM_Var", error );

    if ( error != 0 )
      return free_and_return( library, master, make_unique_face() );

    // Select arbitrary coordinates:
    for ( auto  i = 0u;
          i < master->num_axis &&
            i < AXIS_INDEX_MAX;
          i++ )
      coords.push_back( ( master->axis[i].minimum +
                          master->axis[i].def ) / 2 );

    WARN_ABOUT_IGNORED_VALUES( master->num_axis, AXIS_INDEX_MAX, "axis" );

    error = FT_Set_Var_Design_Coordinates( face.get(),
                                           coords.size(),
                                           coords.data() );
    LOG_FT_ERROR( "FT_Set_Var_Design_Coordinates", error );

    if ( error != 0 )
      return free_and_return( library, master, make_unique_face() );

    return free_and_return( library, master, std::move( face ) );
  }


  freetype::Unique_FT_Face
  freetype::FacePrepIteratorMultipleMasters::
  free_and_return( FT_Library      library,
                   FT_MM_Var*      master,
                   Unique_FT_Face  face )
  {
    (void) FT_Done_MM_Var( library, master );
    return face;
  }
