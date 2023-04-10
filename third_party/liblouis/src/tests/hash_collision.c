/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2013,2015 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include "liblouis.h"
#include "brl_checks.h"

int
main (int argc, char **argv)
{
  int result = 0;
  char *table = "tests/tables/empty.ctb";
  char rule[18];

  lou_compileString(table, "include tables/latinLetterDef6Dots.uti");

  for (char c1 = 'a'; c1 <= 'z'; c1++) {
    for (char c2 = 'a'; c2 <= 'z'; c2++) {
      for (char c3 = 'a'; c3 <= 'z'; c3++) {
	sprintf(rule, "always aa%c%c%c 1", c1, c2, c3);
	lou_compileString(table, rule);
      }
    }
  }

  result |= check(table, "aaaaa", "a");
  result |= check(table, "aaaaa", "a");
  result |= check(table, "aazzz", "a");

  lou_free();
  return result;
}
