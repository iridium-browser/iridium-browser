/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2015 Swiss Library for the Blind, Visually Impaired and Print Disabled
Copyright (C) 2016 Bert Frees <bertfrees@gmail.com>
Copyright (C) 2016 Davy Kager <mail@davykager.nl>

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"
#include "brl_checks.h"

static const char** emph_classes = NULL;

static formtype *
typeform(const char* class, const char* fromString)
{
	int i;
	typeforms kind = plain_text;
	for (i = 0; emph_classes[i]; i++) {
		if (strcmp(class, emph_classes[i]) == 0) {
			switch (i) {
			case 0: kind = italic; break;
			case 1: kind = underline; break;
			case 2: kind = bold; break;
			case 3: kind = emph_4; break;
			case 4: kind = emph_5; break;
			case 5: kind = emph_6; break;
			case 6: kind = emph_7; break;
			case 7: kind = emph_8; break;
			case 8: kind = emph_9; break;
			case 9: kind = emph_10; break;
			default:
				fprintf(stderr, "CODING ERROR\n");
				exit(1);
			}
			break;
		}
	}
	if (kind == plain_text)
		fprintf(stderr, "Warning: typeform '%s' was not declared\n", class);
	formtype *typeform = calloc(strlen(fromString), sizeof(formtype));
	update_typeform(fromString, typeform, kind);
	return typeform;
}

int
main (int argc, char **argv)
{
	int result = 0;
	const char* table;
	static formtype *formtype;
	table = "tests/tables/emphclass/emphclass_invalid_1.utb";
	if (lou_getTable(table)) {
		fprintf(stderr, "%s should be invalid\n", table);
		return 1;
	}
	table = "tests/tables/emphclass/emphclass_invalid_2.utb";
	if (lou_getTable(table)) {
		fprintf(stderr, "%s should be invalid\n", table);
		return 1;
	}
	table = "tests/tables/emphclass/emphclass_valid.utb";
	if (!lou_getTable(table)) {
		fprintf(stderr, "%s should be valid\n", table);
		return 1;
	}
	emph_classes = lou_getEmphClasses(table);
	formtype = typeform("foo", "++++++");
	result |= check(table, "foobar", "~,foobar", .typeform=formtype);
	if (emph_classes) free(emph_classes);
	free(formtype);
	lou_free();
	return result;
}
