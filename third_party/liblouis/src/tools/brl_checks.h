/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2012 James Teh <jamie@nvaccess.org>
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
 *
 * Functionality to check a translation. This is mostly needed for the
 * tests in ../tests but it is also needed for lou_checkyaml. So this
 * functionality is packaged up in what automake calls a convenience
 * library, a lib that is solely built at compile time but never
 * installed.
 */

#include "liblouis.h"

/** Optional parameters for a test
 *
 * Used to define optional and named arguments for the check() macro
 */
typedef struct {
	const char *display_table;
	const formtype *typeform;
	const int cursorPos;
	const int mode;
	const int direction;
	const int diagnostics;
	const int *expected_inputPos;
	const int *expected_outputPos;
	const int expected_cursorPos;
	const int max_outlen;
	const int real_inlen;
} optional_test_params;

#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARD 1
#define DIRECTION_BOTH 2

/** Check a translation
 *
 * Check if an input string is translated as expected.
 *
 * @param tableList comma separated list of tables
 * @param input string to translate
 * @param expected expected output
 * @param display_table (optional) the display table to use (comma separated list of
 * files). If not specified the translation table is used.
 * @param typeform (optional) the typeform for this translation. If not specified it
 * defaults to NULL.
 * @param mode (optional) the translation mode. If not specified it defaults to 0.
 * @param expected_inputPos (optional) the expected input positions. If not specified
 * it defaults to NULL.
 * @param expected_outputPos (optional) the expected input positions. If not specified
 * it defaults to NULL.
 * @param cursorPos (optional) the cursor position for this translation. If not specified
 * it defaults to -1.
 * @param expected_cursorPos (optional) the expected cursor position after this
 * translation. If not specified it defaults to -1.
 * @param max_outlen (optional) the maximum length of the output. If not specified it
 * defaults to -1.
 * @param real_inlen (optional) the length of the portion of the input corresponding to
 * the returned output. Defaults to -1. May only be specified if max_outlen is also
 * specified, and must be smaller than or equal to the total length of the input.
 * @param direction (optional) 0 for forward translation, 1 for backwards translation,
 * 2 for both directions. If
 * not specified it defaults to 0.
 * @param diagnostics (optional) Print diagnostic output on failure if diagnostics is not
 * 0. If not specified it defaults to 1.
 * @return Return 0 if the translation is as expected and 1 otherwise.
 *
 * An example how to invoke this is shown below:
 * ~~~~~~~~~~~~~~~~~~~~~~
 * result = check(table, word, expected_translation,
 *                .typeform = typeform,
 *                .cursorPos = 5);
 * ~~~~~~~~~~~~~~~~~~~~~~
 */
#define check(table, input, expected, ...)                 \
	check_base(table, input, expected,                     \
			(optional_test_params){ .display_table = NULL, \
					.typeform = NULL,                      \
					.cursorPos = -1,                       \
					.expected_cursorPos = -1,              \
					.expected_inputPos = NULL,             \
					.expected_outputPos = NULL,            \
					.max_outlen = -1,                      \
					.real_inlen = -1,                      \
					.mode = 0,                             \
					.direction = DIRECTION_FORWARD,        \
					.diagnostics = 1,                      \
					__VA_ARGS__ })

int
check_base(const char *tableList, const char *input, const char *expected,
		optional_test_params in);

/** Check the cursor positions for a translation
 *
 * For a given input string iterate over all initial cursor positions
 * and check if the returned cursor position equals the one in
 * expected_pos at the same index.
 *
 * Note: This check always translates with compbrlAtCursor and does
 * not check the translation. This would not make sense anyway as the
 * translation changes depending on the initial cursor position. For
 * that reason this function is no longer used in checkyaml.
 *
 * @return 0 if the cursor position for each initial position in the
 * input string equals the one in expected_pos at the same index and 1
 * otherwise.
 * @deprecated use the check() function instead
 */
int
check_cursor_pos(const char *tableList, const char *str, const int *expected_pos);

/** Check if a display table maps characters to the right dots.
 *
 * The dots are read as Unicode braille. Multiple input characters are
 * allowed to map to the same dot pattern. Virtual dots in the actual
 * output are discarded.
 *
 * @return 0 if the result is as expected and 1 otherwise.
 */
int
check_display(const char *displayTableList, const char *input, const char *expected);

/* Check if a string is hyphenated as expected, by passing the
 * expected hyphenation position array.
 *
 * @return 0 if the hyphenation is as expected and 1 otherwise.
 */
int
check_hyphenation_pos(const char *tableList, const char *str, const char *expected);

/** Check if a string is hyphenated as expected.
 *
 * mode is '0' when input is text and '1' when input is braille
 *
 * @return 0 if the hyphenation is as expected and 1 otherwise.
 */
int
check_hyphenation(const char *tableList, const char *str, const char *expected, int mode);

/** Helper function to convert a typeform string to the required format
 *
 * In other words convert a string of '0's, '1's, '2's etc. to an
 * array of 0s, 1s, 2s, etc. For example, "0000011111000" is converted
 * to {0,0,0,0,0,1,1,1,1,1,0,0,0}.
 *
 * The caller is responsible for freeing the returned array.
 */
formtype *
convert_typeform(const char *typeform_string);

void
update_typeform(const char *typeform_string, formtype *typeform, typeforms kind);

int
parsed_strlen(char *s);
