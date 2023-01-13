/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2008 Eitan Isaacson <eitan@ascender.com>
   Copyright (C) 2012 James Teh <jamie@nvaccess.org>
   Copyright (C) 2012 Bert Frees <bertfrees@gmail.com>
   Copyright (C) 2014 Mesar Hameed <mesar.hameed@gmail.com>
   Copyright (C) 2015 Mike Gray <mgray@aph.org>
   Copyright (C) 2010-2017 Swiss Library for the Blind, Visually Impaired and Print
   Disabled
   Copyright (C) 2016-2017 Davy Kager <mail@davykager.nl>

   Copying and distribution of this file, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved. This file is offered as-is,
   without any warranty.
*/

/**
 * @file
 * @brief Test helper functions
 */

#include <config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblouis.h"
#include "internal.h"
#include "brl_checks.h"
#include "unistr.h"

/* see http://stackoverflow.com/questions/5117393/utf-8-strings-length-in-linux-c */
static int
my_strlen_utf8_c(char *s) {
	int i = 0, j = 0;
	while (s[i]) {
		if ((s[i] & 0xc0) != 0x80) j++;
		i++;
	}
	return j;
}

/*
 * String parsing is also done later in check_base. At this point we
 * only need it to compute the actual string length in order to be
 * able to provide error messages when parsing typeform and position arrays.
 */
int
parsed_strlen(char *s) {
	widechar *buf;
	int len, maxlen;
	maxlen = my_strlen_utf8_c(s);
	buf = malloc(sizeof(widechar) * maxlen);
	len = _lou_extParseChars(s, buf);
	free(buf);
	return len;
}

static void
print_int_array(const char *prefix, int *pos_list, int len) {
	int i;
	fprintf(stderr, "%s ", prefix);
	for (i = 0; i < len; i++) fprintf(stderr, "%d ", pos_list[i]);
	fprintf(stderr, "\n");
}

static void
print_typeform(const formtype *typeform, int len) {
	int i;
	fprintf(stderr, "Typeform:  ");
	for (i = 0; i < len; i++) fprintf(stderr, "%hi", typeform[i]);
	fprintf(stderr, "\n");
}

static void
print_widechars(widechar *buffer, int length) {
	uint8_t *result_buf;
	size_t result_len;

#ifdef WIDECHARS_ARE_UCS4
	result_buf = u32_to_u8(buffer, length, NULL, &result_len);
#else
	result_buf = u16_to_u8(buffer, length, NULL, &result_len);
#endif
	fprintf(stderr, "%.*s", (int)result_len, result_buf);
	free(result_buf);
}

/* direction, 0=forward, 1=backwards, 2=both directions. If diagnostics is 1 then
 * print diagnostics in case where the translation is not as
 * expected */

int
check_base(const char *tableList, const char *input, const char *expected,
		optional_test_params in) {

	int i, retval = 0;
	int direction = in.direction;
	const int *expected_inputPos = in.expected_inputPos;
	const int *expected_outputPos = in.expected_outputPos;
	if (in.direction < 0 || in.direction > 2) {
		fprintf(stderr, "Invalid direction.\n");
		return 1;
	}
	if (in.direction != 0 && in.typeform != NULL) {
		// Currently, in backward translation, nothing is done with the initial value of
		// the typeform argument, and on return it always contains all zeros, so it
		// doesn't make any sense to use typeforms in backward translation tests.
		fprintf(stderr, "typeforms only supported with testmode 'forward'\n");
		return 1;
	}
	if (in.direction == 2 && in.cursorPos >= 0) {
		fprintf(stderr, "cursorPos not supported with testmode 'bothDirections'\n");
		return 1;
	}
	if (in.direction == 2 && in.max_outlen >= 0) {
		fprintf(stderr, "maxOutputLength not supported with testmode 'bothDirections'\n");
		return 1;
	}
	if (in.real_inlen >= 0 && in.max_outlen < 0) {
		fprintf(stderr,
				"realInputLength not supported when maxOutputLength is not specified\n");
		return 1;
	}
	while (1) {
		widechar *inbuf, *outbuf, *expectedbuf;
		int inlen = parsed_strlen(input);
		int actualInlen;
		const int outlen_multiplier = 4 + sizeof(widechar) * 2;
		int outlen = inlen * outlen_multiplier;
		int expectedlen = strlen(expected);
		int funcStatus = 0;
		formtype *typeformbuf = NULL;
		int *inputPos = NULL;
		int *outputPos = NULL;
		int cursorPos = 0;
		inbuf = malloc(sizeof(widechar) * inlen);
		outbuf = malloc(sizeof(widechar) * outlen);
		expectedbuf = malloc(sizeof(widechar) * expectedlen);
		if (in.typeform != NULL) {
			typeformbuf = malloc(outlen * sizeof(formtype));
			memcpy(typeformbuf, in.typeform, inlen * sizeof(formtype));
		}
		if (in.cursorPos >= 0) {
			cursorPos = in.cursorPos;
		}
		if (in.max_outlen >= 0) {
			outlen = in.max_outlen;
		}
		inlen = _lou_extParseChars(input, inbuf);
		if (!inlen) {
			fprintf(stderr, "Cannot parse input string.\n");
			retval = 1;
			goto fail;
		}
		if (in.real_inlen > inlen) {
			fprintf(stderr,
					"expected realInputLength (%d) may not exceed total input length "
					"(%d)\n",
					in.real_inlen, inlen);
			return 1;
		}
		if (expected_inputPos) {
			inputPos = malloc(sizeof(int) * outlen);
		}
		if (expected_outputPos) {
			outputPos = malloc(sizeof(int) * inlen);
		}
		actualInlen = inlen;
		// Note that this loop is not strictly needed to make the current tests pass, but
		// in the general case it is needed because it is theoretically possible that we
		// provided a too short output buffer.
		for (int k = 1; k <= 3; k++) {
			if (direction == 1) {
				funcStatus = _lou_backTranslate(tableList, in.display_table, inbuf,
						&actualInlen, outbuf, &outlen, typeformbuf, NULL, outputPos,
						inputPos, &cursorPos, in.mode, NULL, NULL);
			} else {
				funcStatus = _lou_translate(tableList, in.display_table, inbuf,
						&actualInlen, outbuf, &outlen, typeformbuf, NULL, outputPos,
						inputPos, &cursorPos, in.mode, NULL, NULL);
			}
			if (!funcStatus) {
				fprintf(stderr, "Translation failed.\n");
				retval = 1;
				goto fail;
			}
			if (in.max_outlen >= 0 || inlen == actualInlen) {
				break;
			} else if (k < 3) {
				// Hm, something is not quite right. Try again with a larger outbuf
				free(outbuf);
				outlen = inlen * outlen_multiplier * (k + 1);
				outbuf = malloc(sizeof(widechar) * outlen);
				if (expected_inputPos) {
					free(inputPos);
					inputPos = malloc(sizeof(int) * outlen);
				}
				fprintf(stderr,
						"Warning: For %s: returned inlen (%d) differs from passed inlen "
						"(%d) "
						"using outbuf of size %d. Trying again with bigger outbuf "
						"(%d).\n",
						input, actualInlen, inlen, inlen * outlen_multiplier * k, outlen);
				actualInlen = inlen;
			}
		}
		expectedlen = _lou_extParseChars(expected, expectedbuf);
		for (i = 0; i < outlen && i < expectedlen && expectedbuf[i] == outbuf[i]; i++)
			;
		if (i < outlen || i < expectedlen) {
			retval = 1;
			if (in.diagnostics) {
				outbuf[outlen] = 0;
				fprintf(stderr, "Input:    '%s'\n", input);
				/* Print the original typeform not the typeformbuf, as the
				 * latter has been modified by the translation and contains some
				 * information about outbuf */
				if (in.typeform != NULL) print_typeform(in.typeform, inlen);
				if (in.cursorPos >= 0) fprintf(stderr, "Cursor:   %d\n", in.cursorPos);
				fprintf(stderr, "Expected: '%s' (length %d)\n", expected, expectedlen);
				fprintf(stderr, "Received: '");
				print_widechars(outbuf, outlen);
				fprintf(stderr, "' (length %d)\n", outlen);

				uint8_t *expected_utf8;
				uint8_t *out_utf8;
				size_t expected_utf8_len;
				size_t out_utf8_len;
#ifdef WIDECHARS_ARE_UCS4
				expected_utf8 = u32_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
				out_utf8 = u32_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#else
				expected_utf8 = u16_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
				out_utf8 = u16_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#endif

				if (i < outlen && i < expectedlen) {
					fprintf(stderr,
							"Diff:     Expected '%.*s' but received '%.*s' in index %d\n",
							(int)expected_utf8_len, expected_utf8, (int)out_utf8_len,
							out_utf8, i);
				} else if (i < expectedlen) {
					fprintf(stderr,
							"Diff:     Expected '%.*s' but received nothing in index "
							"%d\n",
							(int)expected_utf8_len, expected_utf8, i);
				} else {
					fprintf(stderr,
							"Diff:     Expected nothing but received '%.*s' in index "
							"%d\n",
							(int)out_utf8_len, out_utf8, i);
				}
				free(expected_utf8);
				free(out_utf8);
			}
		}
		if (expected_inputPos) {
			int error_printed = 0;
			for (i = 0; i < outlen; i++) {
				if (expected_inputPos[i] != inputPos[i]) {
					retval = 1;
					if (in.diagnostics) {
						if (!error_printed) {  // Print only once
							fprintf(stderr, "Input position failure:\n");
							error_printed = 1;
						}
						fprintf(stderr, "Expected %d, received %d in index %d\n",
								expected_inputPos[i], inputPos[i], i);
					}
				}
			}
		}
		if (expected_outputPos) {
			int error_printed = 0;
			for (i = 0; i < inlen; i++) {
				if (expected_outputPos[i] != outputPos[i]) {
					retval = 1;
					if (in.diagnostics) {
						if (!error_printed) {  // Print only once
							fprintf(stderr, "Output position failure:\n");
							error_printed = 1;
						}
						fprintf(stderr, "Expected %d, received %d in index %d\n",
								expected_outputPos[i], outputPos[i], i);
					}
				}
			}
		}
		if ((in.expected_cursorPos >= 0) && (cursorPos != in.expected_cursorPos)) {
			retval = 1;
			if (in.diagnostics) {
				fprintf(stderr, "Cursor position failure:\n");
				fprintf(stderr, "Initial:%d Expected:%d Actual:%d \n", in.cursorPos,
						in.expected_cursorPos, cursorPos);
			}
		}
		if (in.max_outlen < 0 && inlen != actualInlen) {
			retval = 1;
			if (in.diagnostics) {
				fprintf(stderr,
						"Unexpected error happened: input length is not the same before "
						"as "
						"after the translation:\n");
				fprintf(stderr, "Before: %d After: %d \n", inlen, actualInlen);
			}
		} else if (actualInlen > inlen) {
			retval = 1;
			if (in.diagnostics) {
				fprintf(stderr,
						"Unexpected error happened: returned input length (%d) exceeds "
						"total input length (%d)\n",
						actualInlen, inlen);
			}
		} else if (in.real_inlen >= 0 && in.real_inlen != actualInlen) {
			retval = 1;
			if (in.diagnostics) {
				fprintf(stderr, "Real input length failure:\n");
				fprintf(stderr, "Expected: %d, received: %d\n", in.real_inlen,
						actualInlen);
			}
		}

	fail:
		free(inbuf);
		free(outbuf);
		free(expectedbuf);
		free(typeformbuf);
		free(inputPos);
		free(outputPos);

		if (direction == 2) {
			const char *tmp = input;
			input = expected;
			expected = tmp;
			expected_inputPos = in.expected_outputPos;
			expected_outputPos = in.expected_inputPos;
			direction = 1;
			continue;
		} else {
			break;
		}
	}

	return retval;
}

/* Helper function to convert a typeform string of '0's, '1's, '2's etc.
 * to the required format, which is an array of 0s, 1s, 2s, etc.
 * For example, "0000011111000" is converted to {0,0,0,0,0,1,1,1,1,1,0,0,0}
 * The caller is responsible for freeing the returned array. */
formtype *
convert_typeform(const char *typeform_string) {
	int len = strlen(typeform_string);
	formtype *typeform = malloc(len * sizeof(formtype));
	int i;
	for (i = 0; i < len; i++) typeform[i] = typeform_string[i] - '0';
	return typeform;
}

void
update_typeform(const char *typeform_string, formtype *typeform, const typeforms kind) {
	int len = strlen(typeform_string);
	int i;
	for (i = 0; i < len; i++)
		if (typeform_string[i] != ' ') typeform[i] |= kind;
}

int
check_cursor_pos(const char *tableList, const char *str, const int *expected_pos) {
	widechar *inbuf;
	widechar *outbuf;
	int *inpos, *outpos;
	int inlen = strlen(str);
	int outlen = inlen;
	int cursor_pos;
	int i, retval = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	outbuf = malloc(sizeof(widechar) * inlen);
	inpos = malloc(sizeof(int) * inlen);
	outpos = malloc(sizeof(int) * inlen);
	inlen = _lou_extParseChars(str, inbuf);

	for (i = 0; i < inlen; i++) {
		cursor_pos = i;
		if (!lou_translate(tableList, inbuf, &inlen, outbuf, &outlen, NULL, NULL, NULL,
					NULL, &cursor_pos, compbrlAtCursor)) {
			fprintf(stderr, "Translation failed.\n");
			retval = 1;
			goto fail;
		}
		if (expected_pos[i] != cursor_pos) {
			if (!retval)  // Print only once
				fprintf(stderr, "Cursorpos failure:\n");
			fprintf(stderr,
					"string='%s' cursor=%d ('%c') expected=%d received=%d ('%c')\n", str,
					i, str[i], expected_pos[i], cursor_pos, (char)outbuf[cursor_pos]);
			retval = 1;
		}
	}

fail:
	free(inbuf);
	free(outbuf);
	free(inpos);
	free(outpos);
	return retval;
}

/** Check if a display table maps characters to the right dots.
 *
 * The dots are read as Unicode braille. Multiple input characters are
 * allowed to map to the same dot pattern. Virtual dots in the actual
 * output are discarded.
 *
 * @return 0 if the result is as expected and 1 otherwise.
 */
int
check_display(const char *displayTableList, const char *input, const char *expected) {
	widechar *inbuf = NULL;
	widechar *outbuf = NULL;
	widechar *expectedbuf = NULL;
	int retval = 0;
	int inlen = strlen(input);
	inbuf = malloc(sizeof(widechar) * inlen);
	inlen = _lou_extParseChars(input, inbuf);
	if (!inlen) {
		fprintf(stderr, "Cannot parse input string.\n");
		retval = 1;
		goto fail;
	}
	int expectedlen = strlen(expected);
	expectedbuf = malloc(sizeof(widechar) * expectedlen);
	expectedlen = _lou_extParseChars(expected, expectedbuf);
	if (!expectedlen) {
		fprintf(stderr, "Cannot parse output string.\n");
		retval = 1;
		goto fail;
	}
	if (inlen != expectedlen) {
		fprintf(stderr, "Input and output string must be the same length.\n");
		retval = 1;
		goto fail;
	}
	for (int i = 0; i < expectedlen; i++) {
		if ((expectedbuf[i] & 0xff00) != LOU_ROW_BRAILLE) {
			fprintf(stderr, "Output string must be Unicode braille.\n");
			retval = 1;
			goto fail;
		}
	}
	outbuf = malloc(sizeof(widechar) * inlen);
	if (!lou_charToDots(displayTableList, inbuf, outbuf, inlen, ucBrl)) {
		// This should only happen if the table can not be compiled.
		// If the table does not have a display rule for a character
		// in the input, it will result in a blank dot pattern in the
		// output.
		fprintf(stderr, "Mapping to dots failed.\n");
		retval = 1;
		goto fail;
	}
	for (int i = 0; i < inlen; i++) {
		if (outbuf[i] != expectedbuf[i]) {
			retval = 1;
			fprintf(stderr, "Input:    '%s'\n", input);
			fprintf(stderr, "Expected: '%s'\n", expected);
			fprintf(stderr, "Received: '");
			print_widechars(outbuf, inlen);
			fprintf(stderr, "'\n");
			uint8_t *expected_utf8;
			uint8_t *out_utf8;
			size_t expected_utf8_len;
			size_t out_utf8_len;
#ifdef WIDECHARS_ARE_UCS4
			expected_utf8 = u32_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
			out_utf8 = u32_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#else
			expected_utf8 = u16_to_u8(&expectedbuf[i], 1, NULL, &expected_utf8_len);
			out_utf8 = u16_to_u8(&outbuf[i], 1, NULL, &out_utf8_len);
#endif
			expectedbuf[i] = (expectedbuf[i] & 0x00ff) | LOU_DOTS;
			outbuf[i] = (outbuf[i] & 0x00ff) | LOU_DOTS;
			fprintf(stderr, "Diff:     Expected '%.*s' (dots %s)", (int)expected_utf8_len,
					expected_utf8, _lou_showDots(&expectedbuf[i], 1));
			fprintf(stderr, " but received '%.*s' (dots %s) in index %d\n",
					(int)out_utf8_len, out_utf8, _lou_showDots(&outbuf[i], 1), i);
			free(expected_utf8);
			free(out_utf8);
			break;
		}
	}
fail:
	free(inbuf);
	free(outbuf);
	free(expectedbuf);
	return retval;
}

/* Check if a string is hyphenated as expected, by passing the
 * expected hyphenation position array.
 *
 * @return 0 if the hyphenation is as expected and 1 otherwise.
 */
int
check_hyphenation_pos(const char *tableList, const char *str, const char *expected) {
	widechar *inbuf;
	char *hyphens = NULL;
	int inlen = strlen(str);
	int retval = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	inlen = _lou_extParseChars(str, inbuf);
	if (!inlen) {
		fprintf(stderr, "Cannot parse input string.\n");
		retval = 1;
		goto fail;
	}
	hyphens = calloc(inlen + 1, sizeof(char));

	if (!lou_hyphenate(tableList, inbuf, inlen, hyphens, 0)) {
		fprintf(stderr, "Hyphenation failed.\n");
		retval = 1;
		goto fail;
	}

	if (strcmp(expected, hyphens)) {
		fprintf(stderr, "Input:    '%s'\n", str);
		fprintf(stderr, "Expected: '%s'\n", expected);
		fprintf(stderr, "Received: '%s'\n", hyphens);
		retval = 1;
	}

fail:
	free(inbuf);
	free(hyphens);
	return retval;
}

/** Check if a string is hyphenated as expected.
 *
 * mode is '0' when input is text and '1' when input is braille
 *
 * @return 0 if the hyphenation is as expected and 1 otherwise.
 */
int
check_hyphenation(
		const char *tableList, const char *str, const char *expected, int mode) {
	widechar *inbuf;
	widechar *hyphenatedbuf = NULL;
	uint8_t *hyphenated = NULL;
	char *hyphens = NULL;
	int inlen = strlen(str);
	size_t hyphenatedlen = inlen * 2;
	int retval = 0;

	inbuf = malloc(sizeof(widechar) * inlen);
	inlen = _lou_extParseChars(str, inbuf);
	if (!inlen) {
		fprintf(stderr, "Cannot parse input string.\n");
		retval = 1;
		goto fail;
	}
	hyphens = calloc(inlen + 1, sizeof(char));

	if (!lou_hyphenate(tableList, inbuf, inlen, hyphens, mode)) {
		fprintf(stderr, "Hyphenation failed.\n");
		retval = 1;
		goto fail;
	}
	if (hyphens[0] != '0') {
		fprintf(stderr, "Unexpected output from lou_hyphenate.\n");
		retval = 1;
		goto fail;
	}

	hyphenatedbuf = malloc(sizeof(widechar) * hyphenatedlen);
	int i = 0;
	int j = 0;
	hyphenatedbuf[i++] = inbuf[j++];
	for (; j < inlen; j++) {
		if (hyphens[j] == '2')
			hyphenatedbuf[i++] = (widechar)'|';
		else if (hyphens[j] != '0')
			hyphenatedbuf[i++] = (widechar)'-';
		hyphenatedbuf[i++] = inbuf[j];
	}

#ifdef WIDECHARS_ARE_UCS4
	hyphenated = u32_to_u8(hyphenatedbuf, i, NULL, &hyphenatedlen);
#else
	hyphenated = u16_to_u8(hyphenatedbuf, i, NULL, &hyphenatedlen);
#endif

	if (!hyphenated) {
		fprintf(stderr, "Unexpected error during UTF-8 encoding\n");
		free(hyphenatedbuf);
		retval = 2;
		goto fail;
	}

	if (strlen(expected) != hyphenatedlen ||
			strncmp(expected, (const char *)hyphenated, hyphenatedlen)) {
		fprintf(stderr, "Input:    '%s'\n", str);
		fprintf(stderr, "Expected: '%s'\n", expected);
		fprintf(stderr, "Received: '%.*s'\n", (int)hyphenatedlen, hyphenated);
		retval = 1;
	}

	free(hyphenatedbuf);
	free(hyphenated);

fail:
	free(inbuf);
	free(hyphens);
	return retval;
}
