/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2019 Bert Frees

   Copying and distribution of this file, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved. This file is offered as-is,
   without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"

// Test that the _lou_charToFallbackDots has the same behavior as the NABCC table for the ASCII range
int main (int argc, char **argv) {
	const DisplayTableHeader* table = _lou_getDisplayTable("tables/text_nabcc.dis");
	widechar c;
	for (c = 0; c < 0x85; c++) {
		widechar d1 = _lou_charToFallbackDots(c);
		widechar d2 = _lou_getDotsForChar(c < 0x80 ? c : '?', table);
		if (d1 != d2) {
			fprintf(stderr, "_lou_charToFallbackDots(%s) expected to return %s but got %s\n",
			        strdup(_lou_showString(&c, 1, 1)),
			        strdup(_lou_showDots(&d2, 1)),
			        strdup(_lou_showDots(&d1, 1)));
			lou_free();
			return 1;
		}
	}
	lou_free();
	return 0;
}
