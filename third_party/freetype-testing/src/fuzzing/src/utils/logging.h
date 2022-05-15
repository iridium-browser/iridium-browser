// logging.h
//
//   Switch between different loggers and/or compile them out completely.
//
// Copyright 2018-2019 by
// Armin Hasitzka.
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.


#ifndef UTILS_LOGGING_H_
#define UTILS_LOGGING_H_


#include <iomanip>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H

#ifdef LOGGER_GLOG

#include <glog/logging.h>

#else // LOGGER_GLOG

#include <iostream>

// Note: the semicolon is important to preserve statements like:
// `if ( foo ) LOG( INFO ) << "Hello world!";'

#define LOG( a )       ; if ( 0 ) std::cout
#define LOG_IF( a, b ) ; if ( 0 ) std::cout

#endif // LOGGER_GLOG

#define LOG_FT_ERROR( fn_name, error )                                  \
  LOG_IF( ERROR, error != 0 )                                           \
  << fn_name << " failed: "                                             \
  << "0x" << std::setfill( '0' ) << std::setw( 2 ) << std::hex << error \
  << " (" << FT_Error_String( error ) << ")"


#endif // UTILS_LOGGING_H_
