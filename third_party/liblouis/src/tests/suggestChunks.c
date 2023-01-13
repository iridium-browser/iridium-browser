/* liblouis Braille Translation and Back-Translation Library

Copyright (C) 2017 Swiss Library for the Blind, Visually Impaired and Print Disabled

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved. This file is offered as-is,
without any warranty. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"

extern void loadTable(const char *tableList);
extern int hyphenationEnabled();
extern widechar toLowercase(widechar c);
extern int suggestChunks(widechar *text, widechar *braille, char *hyphen_string);

static int test_toLowercase() {
	widechar upper = 'A';
	widechar lower = 'a';
	return lower != toLowercase(upper);
}

static int check_suggestion(const char* text, const char* braille, const char* expected_hyphen_string) {
	int in_len = strlen(text);
	int out_len = in_len;
	widechar *inbuf = malloc(sizeof(widechar) * (in_len + 1));
	widechar *outbuf = malloc(sizeof(widechar) * (out_len + 1));
	in_len = _lou_extParseChars(text, inbuf);
	out_len = _lou_extParseChars(braille, outbuf);
	inbuf[in_len] = '\0';
	outbuf[out_len] = '\0';
	char *hyphen_string = malloc(sizeof(char) * (in_len + 2));
	int ret;
	if (!suggestChunks(inbuf, outbuf, hyphen_string)) {
		printf("Could not find a solution for %s => %s\n", text, braille);
		ret = 1;
	} else if (strcmp(expected_hyphen_string, hyphen_string) != 0) {
		printf("Expected %s but got %s\n", expected_hyphen_string, hyphen_string);
		ret = 1;
	} else
		ret = 0;
	free(inbuf);
	free(outbuf);
	free(hyphen_string);
	return ret;
}

int main(int argc, char **argv) {
	int result = 0;
	
	loadTable("tests/tables/suggestChunks.ctb");
	
	result |= test_toLowercase();
	result |= hyphenationEnabled();
	
	result |= check_suggestion("foobar", "FUBR", "^00x00$");
	
	// check that this long word does not take ages
	result |= check_suggestion("achtunddreißigtausenddreihundertsiebzehn",
	                           "A4TUNDDR3^IGT1SENDDR3HUNDERTS0BZEHN",
	                           "^x0xxxxxxx0xxxxx0xxxxxxx0xxxxxxxxx0xxxxx$");
	
	// n(or)m|(al)|(lich)tque|(ll)e
	result |= check_suggestion("normallichtquelle",
	                           "N?M:_T'QUEQE",
	                           "^x0x101000xxxx10x$");
	
	return result;
}
