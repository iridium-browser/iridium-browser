/* liblouis Braille Translation and Back-Translation Library

   Based on the Linux screenreader BRLTTY, copyright (C) 1999-2006 by
   The BRLTTY Team

   Copyright (C) 2004, 2005, 2006, 2009
   ViewPlus Technologies, Inc. www.viewplus.com and
   JJB Software, Inc. www.jjb-software.com

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "liblouis.h"
#include "internal.h"
#include <getopt.h>
#include "progname.h"
#include "unistr.h"
#include "version-etc.h"

static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ NULL, 0, NULL, 0 },
};

const char version_etc_copyright[] =
		"Copyright %s %d ViewPlus Technologies, Inc. and JJB Software, Inc.";

#define AUTHORS "John J. Boyer"

static void
print_help(void) {
	printf("\
Usage: %s [OPTIONS]\n",
			program_name);

	fputs("\
This program tests every capability of the liblouis library. It is\n\
completely interactive. \n\n",
			stdout);

	fputs("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n",
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

#define BUFSIZE 256

static char inputBuffer[BUFSIZE];
static const void *validTable = NULL;
static int forwardOnly = 0;
static int backOnly = 0;
static int showPositions = 0;
static int minimalist = 0;
static int outputSize = BUFSIZE;
static int showSizes = 0;
static int enteredCursorPos = -1;
static unsigned int mode;
static char table[BUFSIZE];
static formtype emphasis[BUFSIZE];
static char spacing[BUFSIZE];
static char enteredEmphasis[BUFSIZE];
static char enteredSpacing[BUFSIZE];

static int
getInput(void) {
	int inputLength;
	inputBuffer[0] = 0;
	if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) exit(EXIT_FAILURE);
	inputLength = strlen(inputBuffer) - 1;
	if (inputLength < 0) /* EOF on script */
	{
		lou_free();
		exit(EXIT_SUCCESS);
	}
	inputBuffer[inputLength] = 0;
	return inputLength;
}

static int
getYN(void) {
	printf("? y/n: ");
	getInput();
	if (inputBuffer[0] == 'y') return 1;
	return 0;
}

static void
paramLetters(void) {
	printf("Press one of the letters in parentheses, then enter.\n");
	printf("(t)able, (r)un, (m)ode, (c)ursor, (e)mphasis, (s)pacing, (h)elp,\n");
	printf("(q)uit, (f)orward-only, (b)ack-only, show-(p)ositions m(i)nimal.\n");
	printf("test-(l)engths.\n");
}

static int
getCommands(void) {
	paramLetters();
	do {
		printf("Command: ");
		getInput();
		switch (inputBuffer[0]) {
		case 0:
			break;
		case 't':
			do {
				printf("Enter the name of a table: ");
				getInput();
				strcpy(table, inputBuffer);
			} while ((validTable = lou_getTable(table)) == NULL);
			break;
		case 'r':
			if (validTable == NULL) {
				printf("You must enter a valid table name.\n");
				inputBuffer[0] = 0;
			}
			break;
		case 'm':
			printf("Reset mode");
			if (getYN()) mode = 0;
			printf("No contractions");
			mode |= noContractions * getYN();
			printf("Computer Braille at cursor");
			mode |= compbrlAtCursor * getYN();
			printf("Dots input and output");
			mode |= dotsIO * getYN();
			printf("Computer Braille left of cursor");
			mode |= compbrlLeftCursor * getYN();
			printf("Unicode Braille");
			mode |= ucBrl * getYN();
			printf("No undefined dots");
			mode |= noUndefined * getYN();
			printf("Partial back-translation");
			mode |= partialTrans * getYN();
			break;
		case 'l':
			printf("Do you want to test input and output lengths");
			showSizes = getYN();
			if (!showSizes) {
				outputSize = BUFSIZE;
				break;
			}
			printf("Enter a maximum output size: ");
			getInput();
			outputSize = atoi(inputBuffer);
			if (outputSize < 0 || outputSize > BUFSIZE) {
				printf("Output size must be from 0 tu %d.\n", BUFSIZE);
				outputSize = BUFSIZE;
				showSizes = 0;
			}
			break;
		case 'c':
			printf("Enter a cursor position: ");
			getInput();
			enteredCursorPos = atoi(inputBuffer);
			if (enteredCursorPos < -1 || enteredCursorPos > outputSize) {
				printf("Cursor position must be from -1 to %d.\n", outputSize);
				enteredCursorPos = -1;
			}
			break;
		case 'e':
			printf("(Enter an x to cancel emphasis.)\n");
			printf("Enter an emphasis string: ");
			getInput();
			strcpy(enteredEmphasis, inputBuffer);
			break;
		case 's':
			printf("(Enter an x to cancel spacing.)\n");
			printf("Enter a spacing string: ");
			getInput();
			strcpy(enteredSpacing, inputBuffer);
			break;
		case 'h':
			printf("Commands: action\n");
			printf("(t)able: Enter a table name\n");
			printf("(r)un: run the translation/back-translation loop\n");
			printf("(m)ode: Enter a mode parameter\n");
			printf("(c)ursor: Enter a cursor position\n");
			printf("(e)mphasis: Enter an emphasis string\n");
			printf("(s)pacing: Enter a spacing string\n");
			printf("(h)elp: print this page\n");
			printf("(q)uit: leave the program\n");
			printf("(f)orward-only: do only forward translation\n");
			printf("(b)ack-only: do only back-translation\n");
			printf("show-(p)ositions: show input and output positions\n");
			printf("m(i)nimal: test translator and back-translator with minimal "
				   "parameters\n");
			printf("test-(l)engths: test accuracy of returned lengths\n");
			printf("\n");
			paramLetters();
			break;
		case 'q':
			lou_free();
			exit(EXIT_SUCCESS);
		case 'f':
			printf("Do only forward translation");
			forwardOnly = getYN();
			break;
		case 'b':
			printf("Do only backward translation");
			backOnly = getYN();
			break;
		case 'p':
			printf("Show input and output positions");
			showPositions = getYN();
			break;
		case 'i':
			printf("Test translation/back-translation loop with minimal parameters");
			minimalist = getYN();
			break;
		default:
			printf("Bad choice.\n");
			break;
		}
		if (forwardOnly && backOnly)
			printf("You cannot specify both forward-only and backward-only "
				   "translation.\n");
	} while (inputBuffer[0] != 'r');
	return 1;
}

int
main(int argc, char **argv) {
	uint8_t *charbuf;
	size_t charlen;
	widechar inbuf[BUFSIZE];
	widechar transbuf[BUFSIZE];
	widechar outbuf[BUFSIZE];
	int outputPos[BUFSIZE];
	int inputPos[BUFSIZE];
	int inlen;
	int translen;
	int outlen;
	int cursorPos = -1;
	int realInlen = 0;
	int optc;

	set_program_name(argv[0]);

	while ((optc = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) {
		switch (optc) {
		/* --help and --version exit immediately, per GNU coding standards. */
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
	}

	if (optind < argc) {
		/* Print error message and exit. */
		fprintf(stderr, "%s: extra operand: %s\n", program_name, argv[optind]);
		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	}

	validTable = NULL;
	enteredCursorPos = -1;
	mode = 0;
	while (1) {
		getCommands();
		printf("Type something, press enter, and view the results.\n");
		printf("A blank line returns to command entry.\n");
		if (minimalist)
			while (1) {
				translen = outputSize;
				outlen = outputSize;
				inlen = getInput();
				if (inlen == 0) break;
				if (!(realInlen = _lou_extParseChars(inputBuffer, inbuf))) break;
				inlen = realInlen;
				if (!lou_translateString(
							table, inbuf, &inlen, transbuf, &translen, NULL, NULL, 0))
					break;
				transbuf[translen] = 0;
				printf("Translation:\n");
#ifdef WIDECHARS_ARE_UCS4
				charbuf = u32_to_u8(transbuf, translen, NULL, &charlen);
#else
				charbuf = u16_to_u8(transbuf, translen, NULL, &charlen);
#endif
				printf("%.*s\n", (int)charlen, charbuf);
				free(charbuf);
				if (showSizes)
					printf("input length = %d; output length = %d\n", inlen, translen);
				lou_backTranslateString(
						table, transbuf, &translen, outbuf, &outlen, NULL, NULL, 0);
				printf("Back-translation:\n");
#ifdef WIDECHARS_ARE_UCS4
				charbuf = u32_to_u8(outbuf, outlen, NULL, &charlen);
#else
				charbuf = u16_to_u8(outbuf, outlen, NULL, &charlen);
#endif
				printf("%.*s\n", (int)charlen, charbuf);
				free(charbuf);
				if (showSizes)
					printf("input length = %d; output length = %d.\n", translen, outlen);
				if (outlen == realInlen) {
					int k;
					for (k = 0; k < realInlen; k++)
						if (inbuf[k] != outbuf[k]) break;
					if (k == realInlen) printf("Perfect roundtrip!\n");
				}
			}
		else
			while (1) {
				memset(emphasis, 0, sizeof(formtype) * BUFSIZE);
				{
					size_t k = 0;
					for (k = 0; k < strlen(enteredEmphasis); k++)
						emphasis[k] = (formtype)enteredEmphasis[k] - '0';
					emphasis[k] = 0;
				}
				strcpy(spacing, enteredSpacing);
				cursorPos = enteredCursorPos;
				inlen = getInput();
				if (inlen == 0) break;
				outlen = outputSize;
				if (backOnly) {
					if (!(translen = _lou_extParseChars(inputBuffer, transbuf))) break;
					inlen = realInlen;
				} else {
					translen = outputSize;
					if (!(realInlen = _lou_extParseChars(inputBuffer, inbuf))) break;
					inlen = realInlen;
					if (!lou_translate(table, inbuf, &inlen, transbuf, &translen,
								emphasis, spacing, &outputPos[0], &inputPos[0],
								&cursorPos, mode))
						break;
					transbuf[translen] = 0;
					if (mode & dotsIO) {
						printf("Translation dot patterns:\n");
						printf("%s\n", _lou_showDots(transbuf, translen));
					} else {
						printf("Translation:\n");
#ifdef WIDECHARS_ARE_UCS4
						charbuf = u32_to_u8(transbuf, translen, NULL, &charlen);
#else
						charbuf = u16_to_u8(transbuf, translen, NULL, &charlen);
#endif
						printf("%.*s\n", (int)charlen, charbuf);
						free(charbuf);
						if (showSizes)
							printf("input length = %d; output length = %d\n", inlen,
									translen);
					}
				}
				if (cursorPos != -1) printf("Cursor position: %d\n", cursorPos);
				if (enteredSpacing[0]) printf("Returned spacing: %s\n", spacing);
				if (showPositions) {
					printf("Output positions:\n");
					for (int k = 0; k < inlen; k++) printf("%d ", outputPos[k]);
					printf("\n");
					printf("Input positions:\n");
					for (int k = 0; k < translen; k++) printf("%d ", inputPos[k]);
					printf("\n");
				}
				if (!forwardOnly) {
					if (!lou_backTranslate(table, transbuf, &translen, outbuf, &outlen,
								emphasis, spacing, &outputPos[0], &inputPos[0],
								&cursorPos, mode))
						break;
					printf("Back-translation:\n");
#ifdef WIDECHARS_ARE_UCS4
					charbuf = u32_to_u8(outbuf, outlen, NULL, &charlen);
#else
					charbuf = u16_to_u8(outbuf, outlen, NULL, &charlen);
#endif
					printf("%.*s\n", (int)charlen, charbuf);
					free(charbuf);
					if (showSizes)
						printf("input length = %d; output length = %d\n", translen,
								outlen);
					if (cursorPos != -1) printf("Cursor position: %d\n", cursorPos);
					if (enteredSpacing[0]) printf("Returned spacing: %s\n", spacing);
					if (showPositions) {
						printf("Output positions:\n");
						for (int k = 0; k < translen; k++) printf("%d ", outputPos[k]);
						printf("\n");
						printf("Input positions:\n");
						for (int k = 0; k < outlen; k++) printf("%d ", inputPos[k]);
						printf("\n");
					}
				}
				if (!(forwardOnly || backOnly)) {
					if (outlen == realInlen) {
						int k;
						for (k = 0; k < realInlen; k++)
							if (inbuf[k] != outbuf[k]) break;
						if (k == realInlen) printf("Perfect roundtrip!\n");
					}
				}
			}
	}
	lou_free();
	exit(EXIT_SUCCESS);
}
