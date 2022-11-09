// driver.cpp
//
//   Entry point of the test driver.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "driver/DriverInternals.h"
#include "utils/logging.h"

  int
  main( int     argc,
        char**  argv )
  {

#ifdef LOGGER_GLOG
    (void) google::InitGoogleLogging( argv[0] );
#endif // LOGGER_GLOG

    freetype::DriverInternals internals;

    if ( argc != 3 )
    {
      (void) internals.print_usage();
      return EXIT_FAILURE;
    }

    std::string  input_file_arg( argv[2] );

    std::ifstream  input_file( input_file_arg, std::ios_base::binary );
    if ( input_file.is_open() == false )
    {
      (void) internals.print_error(
        "error: invalid file '" + input_file_arg + "'" );
      return EXIT_FAILURE;
    }

    std::vector<char>  input( ( std::istreambuf_iterator<char>( input_file ) ),
                                std::istreambuf_iterator<char>() );

    (void) input_file.close();

    if ( internals.run( argv[1],
                        reinterpret_cast<uint8_t*>( input.data() ),
                        input.size() ) == false )
    {
      (void) internals.print_usage();
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }
