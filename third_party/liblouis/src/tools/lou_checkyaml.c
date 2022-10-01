/* liblouis Braille Translation and Back-Translation Library

   Copyright (C) 2015, 2016 Christian Egli, Swiss Library for the Blind, Visually Impaired
   and Print Disabled
   Copyright (C) 2017 Bert Frees

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "internal.h"
#include "error.h"
#include "errno.h"
#include "progname.h"
#include "version-etc.h"
#include "brl_checks.h"

static int verbose = 0;
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ "verbose", no_argument, &verbose, 1 },
	{ NULL, 0, NULL, 0 },
};

const char version_etc_copyright[] =
		"Copyright %s %d Swiss Library for the Blind, Visually Impaired and Print "
		"Disabled";

#define AUTHORS "Christian Egli"

#define MODE_TRANSLATION_FORWARD 0
#define MODE_TRANSLATION_BACKWARD 1
#define MODE_TRANSLATION_BOTH_DIRECTIONS 2
#define MODE_HYPHENATION 3
#define MODE_HYPHENATION_BRAILLE 4
#define MODE_DISPLAY 5
#define MODE_DEFAULT MODE_TRANSLATION_FORWARD

static void
print_help(void) {
	printf("\
Usage: %s YAML_TEST_FILE\n",
			program_name);

	fputs("\
Run the tests defined in the YAML_TEST_FILE. Return 0 if all tests pass\n\
or 1 if any of the tests fail. The details of failing tests are printed\n\
to stderr.\n\n",
			stdout);

	fputs("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\
      --verbose       report expected failures\n",
			stdout);

	printf("\n");
	printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);

#ifdef PACKAGE_PACKAGER_BUG_REPORTS
	printf("Report %s bugs to: %s\n", PACKAGE_PACKAGER, PACKAGE_PACKAGER_BUG_REPORTS);
#endif
#ifdef PACKAGE_URL
	printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

#define EXIT_SKIPPED 77

#ifdef HAVE_LIBYAML
#include <yaml.h>

typedef struct {
	const char *name;
	const char *content;  // table content in case of an inline table; NULL means name is
						  // a file
	int location;		  // location in YAML file (line number) where table is defined
	int is_display;		  // whether the table is a display table or a translation table
} table_value;

const char *event_names[] = { "YAML_NO_EVENT", "YAML_STREAM_START_EVENT",
	"YAML_STREAM_END_EVENT", "YAML_DOCUMENT_START_EVENT", "YAML_DOCUMENT_END_EVENT",
	"YAML_ALIAS_EVENT", "YAML_SCALAR_EVENT", "YAML_SEQUENCE_START_EVENT",
	"YAML_SEQUENCE_END_EVENT", "YAML_MAPPING_START_EVENT", "YAML_MAPPING_END_EVENT" };
const char *encoding_names[] = { "YAML_ANY_ENCODING", "YAML_UTF8_ENCODING",
	"YAML_UTF16LE_ENCODING", "YAML_UTF16BE_ENCODING" };

const char *inline_table_prefix = "checkyaml_inline_table_at_line_";

char *file_name;

int errors = 0;
int count = 0;

static char const **emph_classes = NULL;

static void
simple_error(const char *msg, yaml_parser_t *parser, yaml_event_t *event) {
	error_at_line(EXIT_FAILURE, 0, file_name,
			event->start_mark.line ? event->start_mark.line + 1
								   : parser->problem_mark.line + 1,
			"%s", msg);
}

static void
yaml_parse_error(yaml_parser_t *parser) {
	error_at_line(EXIT_FAILURE, 0, file_name, parser->problem_mark.line + 1, "%s",
			parser->problem);
}

static void
yaml_error(yaml_event_type_t expected, yaml_event_t *event) {
	error_at_line(EXIT_FAILURE, 0, file_name, event->start_mark.line + 1,
			"Expected %s (actual %s)", event_names[expected], event_names[event->type]);
}

static char *
read_table_query(yaml_parser_t *parser, const char **table_file_name_check) {
	yaml_event_t event;
	char *query_as_string = malloc(sizeof(char) * MAXSTRING);
	char *p = query_as_string;
	query_as_string[0] = '\0';
	while (1) {
		if (!yaml_parser_parse(parser, &event)) yaml_error(YAML_SCALAR_EVENT, &event);
		if (event.type == YAML_SCALAR_EVENT) {

			// (temporary) feature to check whether the table query matches an expected
			// table
			if (!strcmp((const char *)event.data.scalar.value, "__assert-match")) {
				yaml_event_delete(&event);
				if (!yaml_parser_parse(parser, &event) ||
						(event.type != YAML_SCALAR_EVENT))
					yaml_error(YAML_SCALAR_EVENT, &event);
				*table_file_name_check = strdup((const char *)event.data.scalar.value);
				yaml_event_delete(&event);
			} else {
				if (query_as_string != p) strcat(p++, " ");
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
				strcat(p++, ":");
				yaml_event_delete(&event);
				if (!yaml_parser_parse(parser, &event) ||
						(event.type != YAML_SCALAR_EVENT))
					yaml_error(YAML_SCALAR_EVENT, &event);
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
				yaml_event_delete(&event);
			}
		} else if (event.type == YAML_MAPPING_END_EVENT) {
			yaml_event_delete(&event);
			break;
		} else
			yaml_error(YAML_SCALAR_EVENT, &event);
	}
	return query_as_string;
}

static void
compile_inline_table(const table_value *table) {
	int location = table->location;
	char *p = (char *)table->content;
	char *line_start = p;
	int line_len = 0;
	while (*p) {
		if (*p == 10 || *p == 13) {
			char *line = strndup((const char *)line_start, line_len);
			int error = 0;
			if (!table->is_display)
				error = !_lou_compileTranslationRule(table->name, line);
			else
				error = !_lou_compileDisplayRule(table->name, line);
			if (error)
				error_at_line(EXIT_FAILURE, 0, file_name, location, "Error in table %s",
						table->name);
			location++;
			free(line);
			line_start = p + 1;
			line_len = 0;
		} else {
			line_len++;
		}
		p++;
	}
}

static table_value *
read_table_value(yaml_parser_t *parser, int start_line, int is_display) {
	table_value *table;
	char *table_name = malloc(sizeof(char) * MAXSTRING);
	char *table_content = NULL;
	yaml_event_t event;
	table_name[0] = '\0';
	if (!yaml_parser_parse(parser, &event) ||
			!(event.type == YAML_SEQUENCE_START_EVENT ||
					event.type == YAML_SCALAR_EVENT ||
					event.type == YAML_MAPPING_START_EVENT))
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s, %s or %s (actual %s)",
				event_names[YAML_SEQUENCE_START_EVENT], event_names[YAML_SCALAR_EVENT],
				event_names[YAML_MAPPING_START_EVENT], event_names[event.type]);
	if (event.type == YAML_SEQUENCE_START_EVENT) {
		yaml_event_delete(&event);
		int done = 0;
		char *p = table_name;
		while (!done) {
			if (!yaml_parser_parse(parser, &event)) {
				yaml_parse_error(parser);
			}
			if (event.type == YAML_SEQUENCE_END_EVENT) {
				done = 1;
			} else if (event.type == YAML_SCALAR_EVENT) {
				if (table_name != p) strcat(p++, ",");
				strcat(p, (const char *)event.data.scalar.value);
				p += event.data.scalar.length;
			}
			yaml_event_delete(&event);
		}
	} else if (event.type == YAML_SCALAR_EVENT) {
		yaml_char_t *p = event.data.scalar.value;
		if (*p)
			while (p[1]) p++;
		if (*p == 10 || *p == 13) {
			// If the scalar ends with a newline, assume it is a block
			// scalar, so treat as an inline table. (Is there a proper way
			// to check for a block scalar?)
			sprintf(table_name, "%s%d", inline_table_prefix, start_line);
			table_content = strdup((const char *)event.data.scalar.value);
		} else {
			strcat(table_name, (const char *)event.data.scalar.value);
		}
		yaml_event_delete(&event);
	} else {  // event.type == YAML_MAPPING_START_EVENT
		char *query;
		const char *table_file_name_check = NULL;
		yaml_event_delete(&event);
		query = read_table_query(parser, &table_file_name_check);
		free(table_name);
		table_name = lou_findTable(query);
		free(query);
		if (!table_name)
			error_at_line(EXIT_FAILURE, 0, file_name, start_line,
					"Table query did not match a table");
		if (table_file_name_check) {
			const char *table_file_name = table_name;
			do {
				table_file_name++;
			} while (*table_file_name);
			while (table_file_name >= table_name && *table_file_name != '/' &&
					*table_file_name != '\\')
				table_file_name--;
			if (strcmp(table_file_name_check, table_file_name + 1))
				error_at_line(EXIT_FAILURE, 0, file_name, start_line,
						"Table query did not match expected table: expected '%s' but got "
						"'%s'",
						table_file_name_check, table_file_name + 1);
			free(table_file_name_check);
		}
	}
	table = malloc(sizeof(table_value));
	table->name = table_name;
	table->content = table_content;
	table->location = start_line + 1;
	table->is_display = is_display;
	return table;
}

static void
free_table_value(table_value *table) {
	if (table) {
		free((char *)table->name);
		if (table->content) free((char *)table->content);
		free(table);
	}
}

static char *
read_table(yaml_event_t *start_event, yaml_parser_t *parser, const char *display_table) {
	table_value *v = NULL;
	char *table_name = NULL;
	if (start_event->type != YAML_SCALAR_EVENT ||
			strcmp((const char *)start_event->data.scalar.value, "table"))
		return 0;
	v = read_table_value(parser, start_event->start_mark.line + 1, 0);
	if (v->content)
		compile_inline_table(v);
	else if (!_lou_getTranslationTable(v->name))
		error_at_line(EXIT_FAILURE, 0, file_name, start_event->start_mark.line + 1,
				"Table %s not valid", v->name);
	free(emph_classes);
	emph_classes = lou_getEmphClasses(v->name);	 // get declared emphasis classes
	table_name = strdup((char *)v->name);
	if (!display_table) {
		if (v->content) {
			v->is_display = 1;
			compile_inline_table(v);
		} else if (!_lou_getDisplayTable(v->name))
			error_at_line(EXIT_FAILURE, 0, file_name, start_event->start_mark.line + 1,
					"Table %s not valid", v->name);
	}
	free_table_value(v);
	return table_name;
}

static void
read_flags(yaml_parser_t *parser, int *testmode) {
	yaml_event_t event;
	int parse_error = 1;

	*testmode = MODE_DEFAULT;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_MAPPING_START_EVENT))
		yaml_error(YAML_MAPPING_START_EVENT, &event);

	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (!strcmp((const char *)event.data.scalar.value, "testmode")) {
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			if (!strcmp((const char *)event.data.scalar.value, "forward")) {
				*testmode = MODE_TRANSLATION_FORWARD;
			} else if (!strcmp((const char *)event.data.scalar.value, "backward")) {
				*testmode = MODE_TRANSLATION_BACKWARD;
			} else if (!strcmp((const char *)event.data.scalar.value, "bothDirections")) {
				*testmode = MODE_TRANSLATION_BOTH_DIRECTIONS;
			} else if (!strcmp((const char *)event.data.scalar.value, "hyphenate")) {
				*testmode = MODE_HYPHENATION;
			} else if (!strcmp((const char *)event.data.scalar.value,
							   "hyphenateBraille")) {
				*testmode = MODE_HYPHENATION_BRAILLE;
			} else if (!strcmp((const char *)event.data.scalar.value, "display")) {
				*testmode = MODE_DISPLAY;
			} else {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Testmode '%s' not supported\n", event.data.scalar.value);
			}
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Flag '%s' not supported\n", event.data.scalar.value);
		}
		yaml_event_delete(&event);
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
}

static int
read_xfail(yaml_parser_t *parser) {
	yaml_event_t event;
	/* assume xfail true if there is an xfail key */
	int xfail = 1;
	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		yaml_error(YAML_SCALAR_EVENT, &event);
	if (!strcmp((const char *)event.data.scalar.value, "false") ||
			!strcmp((const char *)event.data.scalar.value, "off"))
		xfail = 0;
	yaml_event_delete(&event);
	return xfail;
}

static translationModes
read_mode(yaml_parser_t *parser) {
	yaml_event_t event;
	translationModes mode = 0;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (!strcmp((const char *)event.data.scalar.value, "noContractions")) {
			mode |= noContractions;
		} else if (!strcmp((const char *)event.data.scalar.value, "compbrlAtCursor")) {
			mode |= compbrlAtCursor;
		} else if (!strcmp((const char *)event.data.scalar.value, "dotsIO")) {
			mode |= dotsIO;
		} else if (!strcmp((const char *)event.data.scalar.value, "compbrlLeftCursor")) {
			mode |= compbrlLeftCursor;
		} else if (!strcmp((const char *)event.data.scalar.value, "ucBrl")) {
			mode |= ucBrl;
		} else if (!strcmp((const char *)event.data.scalar.value, "noUndefined")) {
			mode |= noUndefined;
		} else if (!strcmp((const char *)event.data.scalar.value, "partialTrans")) {
			mode |= partialTrans;
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Mode '%s' not supported\n", event.data.scalar.value);
		}
		yaml_event_delete(&event);
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return mode;
}

static int
parse_number(const char *number, const char *name, int file_line) {
	char *tail;
	errno = 0;

	int val = strtol(number, &tail, 0);
	if (errno != 0)
		error_at_line(EXIT_FAILURE, 0, file_name, file_line,
				"Not a valid %s '%s'. Must be a number\n", name, number);
	if (number == tail)
		error_at_line(EXIT_FAILURE, 0, file_name, file_line,
				"No digits found in %s '%s'. Must be a number\n", name, number);
	return val;
}

static int *
read_inPos(yaml_parser_t *parser, int translen) {
	int *pos = malloc(sizeof(int) * translen);
	int i = 0;
	yaml_event_t event;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (i >= translen)
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many input positions for translation of length %d.", translen);

		pos[i++] = parse_number((const char *)event.data.scalar.value, "input position",
				event.start_mark.line + 1);
		yaml_event_delete(&event);
	}
	if (i < translen)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too few input positions (%i) for translation of length %i\n", i,
				translen);
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return pos;
}

static int *
read_outPos(yaml_parser_t *parser, int wrdlen, int translen) {
	int *pos = malloc(sizeof(int) * wrdlen);
	int i = 0;
	yaml_event_t event;
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (i >= wrdlen)
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many output positions for input string of length %d.", translen);

		pos[i++] = parse_number((const char *)event.data.scalar.value, "output position",
				event.start_mark.line + 1);
		yaml_event_delete(&event);
	}
	if (i < wrdlen)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too few output positions (%i) for input string of length %i\n", i,
				wrdlen);
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_SEQUENCE_END_EVENT)
		yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	yaml_event_delete(&event);
	return pos;
}

static void
read_cursorPos(yaml_parser_t *parser, int *cursorPos, int *expected_cursorPos, int wrdlen,
		int translen) {
	yaml_event_t event;

	if (!yaml_parser_parse(parser, &event) ||
			!(event.type == YAML_SEQUENCE_START_EVENT || event.type == YAML_SCALAR_EVENT))
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s or %s (actual %s)", event_names[YAML_SEQUENCE_START_EVENT],
				event_names[YAML_SCALAR_EVENT], event_names[event.type]);

	if (event.type == YAML_SEQUENCE_START_EVENT) {
		/* it's a sequence: read the two cursor positions (before and after) */
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
			yaml_error(YAML_SCALAR_EVENT, &event);
		*cursorPos = parse_number((const char *)event.data.scalar.value,
				"cursor position", event.start_mark.line + 1);
		if ((0 > *cursorPos) || (*cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Cursor position (%i) outside of input string of length %i\n",
					*cursorPos, wrdlen);
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too few cursor positions, 2 are expected (before and after)\n");
		*expected_cursorPos = parse_number((const char *)event.data.scalar.value,
				"expected cursor position", event.start_mark.line + 1);
		if ((0 > *expected_cursorPos) || (*expected_cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Expected cursor position (%i) outside of output string of length "
					"%i\n",
					*expected_cursorPos, translen);
		yaml_event_delete(&event);
		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_END_EVENT))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Too many cursor positions, only 2 are expected (before and "
					"after)\n");
		yaml_event_delete(&event);
	} else {  // YAML_SCALAR_EVENT
		/* it's just a single value: just read the initial cursor position */
		*cursorPos = parse_number((const char *)event.data.scalar.value,
				"cursor position before", event.start_mark.line + 1);
		if ((0 > *cursorPos) || (*cursorPos >= wrdlen))
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Cursor position (%i) outside of input string of length %i\n",
					*cursorPos, wrdlen);
		*expected_cursorPos = -1;
		yaml_event_delete(&event);
	}
}

static void
read_typeform_string(yaml_parser_t *parser, formtype *typeform, typeforms kind, int len) {
	yaml_event_t event;
	int typeform_len;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		yaml_error(YAML_SCALAR_EVENT, &event);
	typeform_len = strlen((const char *)event.data.scalar.value);
	if (typeform_len != len)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Too many or too few typeforms (%i) for word of length %i\n",
				typeform_len, len);
	update_typeform((const char *)event.data.scalar.value, typeform, kind);
	yaml_event_delete(&event);
}

static formtype *
read_typeforms(yaml_parser_t *parser, int len) {
	yaml_event_t event;
	formtype *typeform = calloc(len, sizeof(formtype));
	int parse_error = 1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_MAPPING_START_EVENT))
		yaml_error(YAML_MAPPING_START_EVENT, &event);
	yaml_event_delete(&event);

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		if (strcmp((const char *)event.data.scalar.value, "computer_braille") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, computer_braille, len);
		} else if (strcmp((const char *)event.data.scalar.value, "no_translate") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, no_translate, len);
		} else if (strcmp((const char *)event.data.scalar.value, "no_contract") == 0) {
			yaml_event_delete(&event);
			read_typeform_string(parser, typeform, no_contract, len);
		} else {
			int i;
			typeforms kind = plain_text;
			for (i = 0; emph_classes[i]; i++) {
				if (strcmp((const char *)event.data.scalar.value, emph_classes[i]) == 0) {
					yaml_event_delete(&event);
					kind = italic << i;
					if (kind > emph_10)
						error_at_line(EXIT_FAILURE, 0, file_name,
								event.start_mark.line + 1,
								"Typeform '%s' was not declared\n",
								event.data.scalar.value);
					read_typeform_string(parser, typeform, kind, len);
					break;
				}
			}
		}
		yaml_event_delete(&event);
	}
	if (!parse_error) yaml_parse_error(parser);

	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
	return typeform;
}

static void
read_options(yaml_parser_t *parser, int testmode, int wordLen, int translationLen,
		int *xfail, translationModes *mode, formtype **typeform, int **inPos,
		int **outPos, int *cursorPos, int *cursorOutPos, int *maxOutputLen,
		int *realInputLen) {
	yaml_event_t event;
	char *option_name;
	int parse_error = 1;

	*mode = 0;
	*xfail = 0;
	*typeform = NULL;
	*inPos = NULL;
	*outPos = NULL;

	while ((parse_error = yaml_parser_parse(parser, &event)) &&
			(event.type == YAML_SCALAR_EVENT)) {
		option_name =
				strndup((const char *)event.data.scalar.value, event.data.scalar.length);

		if (!strcmp(option_name, "xfail")) {
			yaml_event_delete(&event);
			*xfail = read_xfail(parser);
		} else if (!strcmp(option_name, "mode")) {
			yaml_event_delete(&event);
			*mode = read_mode(parser);
		} else if (!strcmp(option_name, "typeform")) {
			if (testmode != MODE_TRANSLATION_FORWARD) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"typeforms only supported with testmode 'forward'\n");
			}
			yaml_event_delete(&event);
			*typeform = read_typeforms(parser, wordLen);
		} else if (!strcmp(option_name, "inputPos")) {
			yaml_event_delete(&event);
			*inPos = read_inPos(parser, translationLen);
		} else if (!strcmp(option_name, "outputPos")) {
			yaml_event_delete(&event);
			*outPos = read_outPos(parser, wordLen, translationLen);
		} else if (!strcmp(option_name, "cursorPos")) {
			if (testmode == MODE_TRANSLATION_BOTH_DIRECTIONS) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"cursorPos not supported with testmode 'bothDirections'\n");
			}
			yaml_event_delete(&event);
			read_cursorPos(parser, cursorPos, cursorOutPos, wordLen, translationLen);
		} else if (!strcmp(option_name, "maxOutputLength")) {
			if (testmode == MODE_TRANSLATION_BOTH_DIRECTIONS) {
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"maxOutputLength not supported with testmode 'bothDirections'\n");
			}
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			*maxOutputLen = parse_number((const char *)event.data.scalar.value,
					"Maximum output length", event.start_mark.line + 1);
			if (*maxOutputLen <= 0)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Maximum output length (%i) must be a positive number\n",
						*maxOutputLen);
			if (*maxOutputLen < translationLen)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected translation length (%i) must not exceed maximum output "
						"length (%i)\n",
						translationLen, *maxOutputLen);
			yaml_event_delete(&event);
		} else if (!strcmp(option_name, "realInputLength")) {
			yaml_event_delete(&event);
			if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
				yaml_error(YAML_SCALAR_EVENT, &event);
			*realInputLen = parse_number((const char *)event.data.scalar.value,
					"Real input length", event.start_mark.line + 1);
			if (*realInputLen < 0)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Real input length (%i) must not be a negative number\n",
						*realInputLen);
			if (*realInputLen > wordLen)
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Real input length (%i) must not exceed total input "
						"length (%i)\n",
						*realInputLen, wordLen);
			yaml_event_delete(&event);
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Unsupported option %s", option_name);
		}
		free(option_name);
	}
	if (!parse_error) yaml_parse_error(parser);
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);
}

static void
read_test(yaml_parser_t *parser, char **tables, const char *display_table, int testmode) {
	yaml_event_t event;
	char *description = NULL;
	char *word;
	char *translation;
	int xfail = 0;
	translationModes mode = 0;
	formtype *typeform = NULL;
	int *inPos = NULL;
	int *outPos = NULL;
	int cursorPos = -1;
	int cursorOutPos = -1;
	int maxOutputLen = -1;
	int realInputLen = -1;

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		simple_error("Word expected", parser, &event);

	word = strndup((const char *)event.data.scalar.value, event.data.scalar.length);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SCALAR_EVENT))
		simple_error("Translation expected", parser, &event);

	translation =
			strndup((const char *)event.data.scalar.value, event.data.scalar.length);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(parser, &event)) yaml_parse_error(parser);

	/* Handle an optional description */
	if (event.type == YAML_SCALAR_EVENT) {
		description = word;
		word = translation;
		translation =
				strndup((const char *)event.data.scalar.value, event.data.scalar.length);
		yaml_event_delete(&event);

		if (!yaml_parser_parse(parser, &event)) yaml_parse_error(parser);
	}

	if (event.type == YAML_MAPPING_START_EVENT) {
		yaml_event_delete(&event);
		read_options(parser, testmode, parsed_strlen(word), parsed_strlen(translation),
				&xfail, &mode, &typeform, &inPos, &outPos, &cursorPos, &cursorOutPos,
				&maxOutputLen, &realInputLen);

		if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_END_EVENT))
			yaml_error(YAML_SEQUENCE_END_EVENT, &event);
	} else if (event.type != YAML_SEQUENCE_END_EVENT) {
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"Expected %s or %s (actual %s)", event_names[YAML_MAPPING_START_EVENT],
				event_names[YAML_SEQUENCE_END_EVENT], event_names[event.type]);
	}

	int result = 0;
	char **table = tables;
	while (*table) {
		int r;
		if (testmode == MODE_HYPHENATION || testmode == MODE_HYPHENATION_BRAILLE) {
			r = check_hyphenation(
					*table, word, translation, testmode == MODE_HYPHENATION_BRAILLE);

		} else if (testmode == MODE_DISPLAY) {
			r = check_display(*table, word, translation);
		} else {
			int direction = DIRECTION_FORWARD;
			if (testmode == MODE_TRANSLATION_BACKWARD)
				direction = DIRECTION_BACKWARD;
			else if (testmode == MODE_TRANSLATION_BOTH_DIRECTIONS)
				direction = DIRECTION_BOTH;
			// FIXME: Note that the typeform array was constructed using the
			// emphasis classes mapping of the last compiled table. This
			// means that if we are testing multiple tables at the same time
			// they must have the same mapping (i.e. the emphasis classes
			// must be defined in the same order).
			r = check(*table, word, translation, .display_table = display_table,
					.typeform = typeform, .mode = mode, .expected_inputPos = inPos,
					.expected_outputPos = outPos, .cursorPos = cursorPos,
					.expected_cursorPos = cursorOutPos, .max_outlen = maxOutputLen,
					.real_inlen = realInputLen, .direction = direction,
					.diagnostics = !xfail);
		}
		if (xfail != r) {
			// FAIL or XPASS
			if (description) fprintf(stderr, "%s\n", description);
			error_at_line(0, 0, file_name, event.start_mark.line + 1,
					(xfail ? "Unexpected Pass" : "Failure"));
			errors++;
			// on error print the table name, as it isn't always clear
			// which table we are testing. You can can define a test
			// for multiple tables.
			fprintf(stderr, "Table: %s\n", *table);
			if (display_table) fprintf(stderr, "Display table: %s\n", display_table);
			// add an empty line after each error
			fprintf(stderr, "\n");
		} else if (xfail && r && verbose) {
			// XFAIL
			// in verbose mode print expected failures
			if (description) fprintf(stderr, "%s\n", description);
			error_at_line(0, 0, file_name, event.start_mark.line + 1, "Expected Failure");
			fprintf(stderr, "Table: %s\n", *table);
			if (display_table) fprintf(stderr, "Display table: %s\n", display_table);
			fprintf(stderr, "\n");
		}
		result |= r;
		table++;
		count++;
	}
	yaml_event_delete(&event);

	free(description);
	free(word);
	free(translation);
	free(typeform);
	free(inPos);
	free(outPos);
}

static void
read_tests(
		yaml_parser_t *parser, char **tables, const char *display_table, int testmode) {
	yaml_event_t event;
	if (!yaml_parser_parse(parser, &event) || (event.type != YAML_SEQUENCE_START_EVENT))
		yaml_error(YAML_SEQUENCE_START_EVENT, &event);

	yaml_event_delete(&event);

	int done = 0;
	while (!done) {
		if (!yaml_parser_parse(parser, &event)) {
			yaml_parse_error(parser);
		}
		if (event.type == YAML_SEQUENCE_END_EVENT) {
			done = 1;
			yaml_event_delete(&event);
		} else if (event.type == YAML_SEQUENCE_START_EVENT) {
			yaml_event_delete(&event);
			read_test(parser, tables, display_table, testmode);
		} else {
			error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
					"Expected %s or %s (actual %s)", event_names[YAML_SEQUENCE_END_EVENT],
					event_names[YAML_SEQUENCE_START_EVENT], event_names[event.type]);
		}
	}
}

/*
 * This custom table resolver handles magic table names that represent
 * inline tables.
 */
static char **
customTableResolver(const char *tableList, const char *base) {
	static char *dummy_table[1];
	static char **ret;
	char *p = (char *)tableList;
	while (*p != '\0') {
		if (strncmp(p, inline_table_prefix, strlen(inline_table_prefix)) == 0)
			return dummy_table;
		while (*p != '\0' && *p != ',') p++;
		if (*p == ',') p++;
	}
	if (ret) {
		for (int i = 0; ret[i]; i++) free(ret[i]);
		free(ret);
	}
	ret = _lou_defaultTableResolver(tableList, base);
	return ret;
}

#endif	// HAVE_LIBYAML

int
main(int argc, char *argv[]) {
	int optc;

	set_program_name(argv[0]);

	while ((optc = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) switch (optc) {
		/* --help and --version exit immediately, per GNU coding standards.  */
		case 'v':
			version_etc(
					stdout, program_name, PACKAGE_NAME, VERSION, AUTHORS, (char *)NULL);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
			exit(EXIT_FAILURE);
			break;
		}

	if (optind != argc - 1) {
		/* Print error message and exit.  */
		if (optind < argc - 1)
			fprintf(stderr, "%s: extra operand: %s\n", program_name, argv[optind + 1]);
		else
			fprintf(stderr, "%s: no YAML test file specified\n", program_name);

		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	}

#ifdef WITHOUT_YAML
	fprintf(stderr,
			"Skipping tests for %s as yaml was disabled in configure with "
			"--without-yaml\n",
			argv[1]);
	return EXIT_SKIPPED;
#else
#ifndef HAVE_LIBYAML
	fprintf(stderr, "Skipping tests for %s as libyaml was not found\n", argv[1]);
	return EXIT_SKIPPED;
#endif	// not HAVE_LIBYAML
#endif	// WITHOUT_YAML

#ifndef WITHOUT_YAML
#ifdef HAVE_LIBYAML

	FILE *file;
	yaml_parser_t parser;
	yaml_event_t event;

	file_name = argv[1];
	file = fopen(file_name, "rb");
	if (!file) {
		fprintf(stderr, "%s: file not found: %s\n", program_name, file_name);
		exit(3);
	}

	char *dir_name = strdup(file_name);
	int i = strlen(dir_name);
	while (i > 0) {
		if (dir_name[i - 1] == '/' || dir_name[i - 1] == '\\') {
			i--;
			break;
		}
		i--;
	}
	dir_name[i] = '\0';
	// FIXME: problem with this is that
	// LOUIS_TABLEPATH=$(top_srcdir)/tables,... does not work anymore because
	// $(top_srcdir) == .. (not an absolute path)
	if (i > 0)
		if (chdir(dir_name))
			error(EXIT_FAILURE, EIO, "Cannot change directory to %s", dir_name);
	free(dir_name);

	// register custom table resolver
	lou_registerTableResolver(&customTableResolver);

	assert(yaml_parser_initialize(&parser));

	yaml_parser_set_input_file(&parser, file);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_STREAM_START_EVENT)) {
		yaml_error(YAML_STREAM_START_EVENT, &event);
	}

	if (event.data.stream_start.encoding != YAML_UTF8_ENCODING)
		error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
				"UTF-8 encoding expected (actual %s)",
				encoding_names[event.data.stream_start.encoding]);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) ||
			(event.type != YAML_DOCUMENT_START_EVENT)) {
		yaml_error(YAML_DOCUMENT_START_EVENT, &event);
	}
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_MAPPING_START_EVENT)) {
		yaml_error(YAML_MAPPING_START_EVENT, &event);
	}
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event))
		simple_error("table expected", &parser, &event);

	int MAXTABLES = 150;
	char *tables[MAXTABLES + 1];
	char *display_table = NULL;
	while (1) {
		if (event.type == YAML_SCALAR_EVENT &&
				!strcmp((const char *)event.data.scalar.value, "display")) {
			table_value *v;
			free(display_table);
			v = read_table_value(&parser, event.start_mark.line + 1, 1);
			display_table = strdup((char *)v->name);
			if (v->content)
				compile_inline_table(v);
			else if (!_lou_getDisplayTable(display_table))
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Display table %s not valid", display_table);
			free_table_value(v);
			yaml_event_delete(&event);
			if (!yaml_parser_parse(&parser, &event))
				simple_error("table expected", &parser, &event);
		}
		if (!(tables[0] = read_table(&event, &parser, display_table))) break;
		yaml_event_delete(&event);
		int k = 1;
		while (1) {
			if (!yaml_parser_parse(&parser, &event))
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected table or %s (actual %s)",
						event_names[YAML_SCALAR_EVENT], event_names[event.type]);
			if ((tables[k++] = read_table(&event, &parser, display_table))) {
				if (k == MAXTABLES)
					error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
							"Only %d tables in one YAML test supported", MAXTABLES);
				yaml_event_delete(&event);
			} else
				break;
		}

		if (event.type != YAML_SCALAR_EVENT) yaml_error(YAML_SCALAR_EVENT, &event);

		int haveRunTests = 0;
		while (1) {
			int testmode = MODE_DEFAULT;
			if (!strcmp((const char *)event.data.scalar.value, "flags")) {
				yaml_event_delete(&event);
				read_flags(&parser, &testmode);

				if (!yaml_parser_parse(&parser, &event) ||
						(event.type != YAML_SCALAR_EVENT) ||
						strcmp((const char *)event.data.scalar.value, "tests")) {
					simple_error("tests expected", &parser, &event);
				}
				yaml_event_delete(&event);
				read_tests(&parser, tables, display_table, testmode);
				haveRunTests = 1;

			} else if (!strcmp((const char *)event.data.scalar.value, "tests")) {
				yaml_event_delete(&event);
				read_tests(&parser, tables, display_table, testmode);
				haveRunTests = 1;
			} else {
				if (haveRunTests) {
					break;
				} else {
					simple_error("flags or tests expected", &parser, &event);
				}
			}
			if (!yaml_parser_parse(&parser, &event))
				error_at_line(EXIT_FAILURE, 0, file_name, event.start_mark.line + 1,
						"Expected table, flags, tests or %s (actual %s)",
						event_names[YAML_MAPPING_END_EVENT], event_names[event.type]);
			if (event.type != YAML_SCALAR_EVENT) break;
		}

		char **p = tables;
		while (*p) free(*(p++));
	}
	if (event.type != YAML_MAPPING_END_EVENT) yaml_error(YAML_MAPPING_END_EVENT, &event);
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_DOCUMENT_END_EVENT)) {
		yaml_error(YAML_DOCUMENT_END_EVENT, &event);
	}
	yaml_event_delete(&event);

	if (!yaml_parser_parse(&parser, &event) || (event.type != YAML_STREAM_END_EVENT)) {
		yaml_error(YAML_STREAM_END_EVENT, &event);
	}
	yaml_event_delete(&event);

	yaml_parser_delete(&parser);

	free(emph_classes);
	free(display_table);
	lou_free();

	assert(!fclose(file));

	printf("%s (%d tests, %d failure%s)\n", (errors ? "FAILURE" : "SUCCESS"), count,
			errors, ((errors != 1) ? "s" : ""));

	return errors ? 1 : 0;

#endif	// HAVE_LIBYAML
#endif	// not WITHOUT_YAML
}
