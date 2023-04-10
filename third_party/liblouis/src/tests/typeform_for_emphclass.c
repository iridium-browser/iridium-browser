/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2016 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2016 Davy Kager <mail@davykager.nl>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "liblouis.h"

int
main (int argc, char **argv)
{
	int result = 0;
	const char *table = "tests/tables/emphclass/emphclass_valid.utb";
	const char *class;
	class = "italic";
	formtype typeform = italic;
	if (lou_getTypeformForEmphClass(table, class) != typeform) {
		fprintf(stderr, "%s should have typeform %x\n", class, typeform);
		result = 1;
	}
	class = "underline";
	typeform = underline;
	if (lou_getTypeformForEmphClass(table, class) != typeform) {
		fprintf(stderr, "%s should have typeform %x\n", class, typeform);
		result = 1;
	}
	class = "bold";
	typeform = bold;
	if (lou_getTypeformForEmphClass(table, class) != typeform) {
		fprintf(stderr, "%s should have typeform %x\n", class, typeform);
		result = 1;
	}
	class = "foo";
	typeform = emph_4;
	if (lou_getTypeformForEmphClass(table, class) != typeform) {
		fprintf(stderr, "%s should have typeform %x\n", class, typeform);
		result = 1;
	}
	class = "bar";
	if (lou_getTypeformForEmphClass(table, class) != 0) {
		fprintf(stderr, "%s should not be defined\n", class);
		result = 1;
	}
	lou_free();
	return result;
}
