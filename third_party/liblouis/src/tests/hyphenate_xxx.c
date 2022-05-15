/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2013 Mesar Hameed <mesar.hameed@gmail.com>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "liblouis.h"
#include "brl_checks.h"

/* The same bug can be reproduced with only 3 'x'es. */
int main(int argc, char **argv)
{
  int ret = 0;
  char *tables = "tables/cs-g1.ctb,tables/hyph_cs_CZ.dic";
  char *word = "xxx";
  char * hyphens = calloc(5, sizeof(char));

  hyphens[0] = '0';
  hyphens[1] = '0';
  hyphens[2] = '0';

  ret = check_hyphenation_pos(tables, word, hyphens);
  assert(hyphens[3] == '\0');
  assert(hyphens[4] == '\0');
  free(hyphens);
  lou_free();
  return ret;
}
