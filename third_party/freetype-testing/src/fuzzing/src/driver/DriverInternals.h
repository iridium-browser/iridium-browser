// DriverInternals.h
//
//   Driver internals.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef DRIVER_DRIVERINTERNALS_H_
#define DRIVER_DRIVERINTERNALS_H_


#include <string>
#include <unordered_map>
#include <vector>


#include "utils/noncopyable.h"


namespace freetype {


  class DriverInternals
    : private noncopyable
  {
  public:


    DriverInternals();


    bool
    run( const std::string& type_arg,
         const uint8_t*     data,
         size_t             size );


    void
    print_error( const std::string&  message );


    void
    print_usage();


  private:


    typedef void (*RunFunc)( const uint8_t*, size_t );

    typedef std::unordered_map<std::string, RunFunc>  FuzzTargets;

    FuzzTargets               targets;
    std::vector<std::string>  usage_types;


    template<class T>
    static void
    run( const uint8_t*  data,
         size_t          size )
    {
      // Late initialisation:
      (void) ( T() ).run( data, size );
    }


    template<class T>
    void
    add( const std::string&  name )
    {
      (void) targets.insert( std::make_pair( "--" + name, run<T> ) );
      (void) usage_types.push_back( name );
    }
  };
}


#endif // DRIVER_DRIVERINTERNALS_H_
