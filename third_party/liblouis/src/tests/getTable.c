/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 James Teh <jamie@nvaccess.org>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"

int
main(int argc, char **argv)
{
  const char* goodTable = "tables/en-us-g1.ctb";
  const char* badTable = "tests/tables/bad.ctb";
  const void* table = NULL;
  int result = 0;

  table = lou_getTable(goodTable);
  if (!table)
    {
      printf("Getting %s failed, expected success\n", goodTable);
      result = 1;
    }
  else
    table = NULL;

  table = lou_getTable(badTable);
  if (table)
    {
      printf("Getting %s succeeded, expected failure\n", badTable);
      table = NULL;
      result = 1;
    }

  table = lou_getTable(goodTable);
  if (!table)
    {
      printf("Getting %s failed, expected success\n", goodTable);
      result = 1;
    }
  else
    table = NULL;

  lou_free();

  return result;
}
