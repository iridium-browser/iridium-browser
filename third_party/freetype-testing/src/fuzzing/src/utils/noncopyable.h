// noncopyable.h
//
//   Helper base class for copy constructor and copy assignment
//   operator deletion.
//
// Copyright 2020 by
// Dominik Roettsches
//
// This file is part of the FreeType project, and may only be used,
// modified, and distributed under the terms of the FreeType project
// license, LICENSE.TXT.  By continuing to use, modify, or distribute
// this file you indicate that you have read the license and
// understand and accept it fully.

#ifndef UTILS_NONCOPYABLE_H_
#define UTILS_NONCOPYABLE_H_


namespace freetype
{


  class noncopyable
  {
  protected:


    noncopyable()  = default;


    ~noncopyable() = default;


    noncopyable( const noncopyable& ) = delete;


    noncopyable&
    operator=( const noncopyable& ) = delete;
  };
}  // namespace freetype

#endif // UTILS_NONCOPYABLE_H_
