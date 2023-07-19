/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2012 James Teh <jamie@nvaccess.org>
Copyright (C) 2015 Davy Kager <mail@davykager.nl>

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
  const char *goodTable = "tables/en-us-g1.ctb";
  const char *badTable = "tests/tables/bad.ctb";
  int result = 0;

  if (lou_checkTable(goodTable) == 0)
  {
    printf("Getting %s failed, expected success\n", goodTable);
    result = 1;
  }

  if (lou_checkTable(badTable) != 0)
  {
    printf("Getting %s succeeded, expected failure\n", badTable);
    result = 1;
  }

  if (lou_checkTable(goodTable) == 0)
  {
    printf("Getting %s failed, expected success\n", goodTable);
    result = 1;
  }

  lou_free();

  return result;
}
