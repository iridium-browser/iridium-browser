/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2020 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"

int
main (int argc, char **argv)
{
	const char* table;
	// the following table contains an invalid attribute name and should not compile
	table = "tests/tables/attribute/attributeName_invalid.utb";
	if (lou_getTable(table)) {
		fprintf(stderr, "%s should be invalid\n", table);
		return 1;
	}
	table = "tests/tables/attribute/attributeName_valid.utb";
	if (!lou_getTable(table)) {
		fprintf(stderr, "%s should be valid\n", table);
		return 1;
	}
	lou_free();
	return 0;
}
