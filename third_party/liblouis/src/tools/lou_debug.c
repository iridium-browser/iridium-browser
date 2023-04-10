/* liblouis Braille Translation and Back-Translation Library

   Based on BRLTTY, copyright (C) 1999-2006 by
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
Usage: %s [OPTIONS] TABLE[,TABLE,...]\n",
			program_name);

	fputs("\
Examine and debug Braille translation tables. This program allows you\n\
to inspect liblouis translation tables and gather information about\n\
them, such as forward and backward rules, characters and dot patterns,\n\
specific opcodes, the size of a table, whether a hyphenation\n\
table is used, how many passes the translation takes and much\n\
more.\n\n",
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

static const TranslationTableHeader *table;
static const DisplayTableHeader *displayTable;
static char inputBuffer[BUFSIZE];

static int
getInput(void) {
	int inputLength;
	inputBuffer[0] = 0;
	if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) exit(EXIT_FAILURE);
	inputLength = strlen(inputBuffer) - 1;
	if (inputLength < 0) /* EOF on script */
		exit(EXIT_FAILURE);
	inputBuffer[inputLength] = 0;
	return inputLength;
}

static char *
print_chars(const widechar *buffer, int length) {
	static uint8_t result_buf[BUFSIZE];
	size_t result_len = BUFSIZE - 1;
#ifdef WIDECHARS_ARE_UCS4
	u32_to_u8(buffer, length, result_buf, &result_len);
#else
	u16_to_u8(buffer, length, result_buf, &result_len);
#endif
	result_buf[result_len] = 0;
	return result_buf;
}

static int
printRule(TranslationTableRule *thisRule, int mode) {
	printf("Rule: ");
	printf("opcode=%s, ", _lou_findOpcodeName(thisRule->opcode));
	if (thisRule->before) printf("before=%x, ", thisRule->before);
	if (thisRule->after) printf("after=%x, ", thisRule->after);
	switch (thisRule->opcode) {
	case CTO_Context:
	case CTO_Correct:
	case CTO_SwapCd:
	case CTO_SwapDd:
	case CTO_Pass2:
	case CTO_Pass3:
	case CTO_Pass4:
		printf("code=%s ",
				print_chars(thisRule->charsdots, thisRule->charslen + thisRule->dotslen));
		break;
	default:
		if (mode == 0) {
			printf("chars=%s, ", print_chars(thisRule->charsdots, thisRule->charslen));
			printf("dots=%s, ",
					_lou_showDots(
							&thisRule->charsdots[thisRule->charslen], thisRule->dotslen));
		} else {
			printf("dots=%s, ",
					_lou_showDots(
							&thisRule->charsdots[thisRule->charslen], thisRule->dotslen));
			printf("chars=%s, ", print_chars(thisRule->charsdots, thisRule->charslen));
		}
		break;
	}
	return 1;
}

static int
printCharacter(TranslationTableCharacter *thisChar, int mode) {
	TranslationTableRule *thisRule;
	TranslationTableOffset nextRule;
	if (mode == 0) {
		printf("Char: ");
		printf("value=%s, ", print_chars(&thisChar->value, 1));
	} else
		printf("Dots: value=%s, ", _lou_showDots(&thisChar->value, 1));
	printf("attr=%s, ", _lou_showAttributes(thisChar->attributes));
	nextRule = thisChar->otherRules;
	while (nextRule) {
		thisRule = (TranslationTableRule *)&table->ruleArea[nextRule];
		if (nextRule == thisChar->definitionRule) printf("definition ");
		printRule(thisRule, mode);
		printf("\n");
		if (mode == 0)
			nextRule = thisRule->charsnext;
		else
			nextRule = thisRule->dotsnext;
	}
	if (mode == 0 && thisChar->compRule) {
		TranslationTableRule *compRule =
				(TranslationTableRule *)&table->ruleArea[thisChar->compRule];
		printf("comp6 ");
		printRule(compRule, 0);
		printf("\n");
	}
	return 1;
}

static int
show_characters(int startHash) {
	int k;
	TranslationTableCharacter *thisChar;
	TranslationTableOffset nextChar;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (table->characters[k]) {
			printf("Hash=%d\n", k);
			nextChar = table->characters[k];
			while (nextChar) {
				thisChar = (TranslationTableCharacter *)&table->ruleArea[nextChar];
				printCharacter(thisChar, 0);
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextChar = thisChar->next;
			}
		}
	return 1;
}

static int
show_dots(int startHash) {
	int k;
	TranslationTableCharacter *thisDots;
	TranslationTableOffset nextDots;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (table->dots[k]) {
			printf("Hash=%d\n", k);
			nextDots = table->dots[k];
			while (nextDots) {
				thisDots = (TranslationTableCharacter *)&table->ruleArea[nextDots];
				printCharacter(thisDots, 1);
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextDots = thisDots->next;
			}
		}
	return 1;
}

static int
show_forRules(int startHash) {
	int k;
	TranslationTableRule *thisRule;
	TranslationTableOffset nextRule;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (table->forRules[k]) {
			printf("Hash=%d\n", k);
			nextRule = table->forRules[k];
			while (nextRule) {
				thisRule = (TranslationTableRule *)&table->ruleArea[nextRule];
				printRule(thisRule, 0);
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextRule = thisRule->charsnext;
			}
		}
	return 1;
}

static int
show_backRules(int startHash) {
	int k;
	TranslationTableRule *thisRule;
	TranslationTableOffset nextRule;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (table->backRules[k]) {
			printf("Hash=%d\n", k);
			nextRule = table->backRules[k];
			while (nextRule) {
				thisRule = (TranslationTableRule *)&table->ruleArea[nextRule];
				printRule(thisRule, 1);
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextRule = thisRule->dotsnext;
			}
		}
	return 1;
}

static int
print_brailleIndicator(TranslationTableOffset offset, const char *opcode) {
	TranslationTableRule *thisRule;
	if (!offset) return 0;
	thisRule = (TranslationTableRule *)&table->ruleArea[offset];
	printf("%s %s\n", opcode, _lou_showDots(&thisRule->charsdots[0], thisRule->dotslen));
	return 1;
}

static int
print_phraseLength(TranslationTableOffset offset, const char *opcode) {
	if (!offset) return 0;
	printf("%s %u\n", opcode, offset);
	return 1;
}

static int
show_brailleIndicators(void) {
	char name[BUFSIZE];
	const char *emphNames[] = { "begemphphrase %s", "endemphphrase %s before",
		"endemphphrase %s after", "begemphword %s", "endemphword %s", "emphletter %s",
		"begemph %s", "endemph %s", NULL };
	const char *capsNames[] = { "firstwordcaps", "lastwordcapsbefore",
		"lastwordcapsafter", "begcaps", "endcaps", "capsletter", "capsword",
		"capswordstop", NULL };

	// FIXME: update to include all UEB opcodes.

	for (EmphCodeOffset offset = 0; capsNames[offset]; offset++) {
		print_brailleIndicator(
				table->emphRules[MAX_EMPH_CLASSES][offset], capsNames[offset]);
	}
	print_phraseLength(
			table->emphRules[MAX_EMPH_CLASSES][lenPhraseOffset], "lencapsphrase");
	print_brailleIndicator(table->letterSign, "letsign");
	print_brailleIndicator(table->numberSign, "numsign");
	print_brailleIndicator(table->noNumberSign, "nonumsign");
	print_brailleIndicator(table->noContractSign, "nocontractsign");

	for (int i = 0; i < MAX_EMPH_CLASSES && table->emphClassNames[i]; i++) {
		for (EmphCodeOffset offset = 0; emphNames[offset]; offset++) {
			snprintf(name, BUFSIZE, emphNames[offset], table->emphClasses[i]);
			print_brailleIndicator(table->emphRules[i][offset], name);
		}
		snprintf(name, BUFSIZE, "lenemphphrase %s", table->emphClasses[i]);
		print_phraseLength(table->emphRules[i][lenPhraseOffset], name);
	}
	print_brailleIndicator(table->begComp, "begcomp");
	print_brailleIndicator(table->endComp, "endcomp");
	return 1;
}

static const char *
pickYN(int a) {
	if (!a) return "no";
	return "yes";
}

static int
show_misc(void) {
	printf("Table size: %u\n", table->tableSize);
	printf("Bytes used: %u\n", table->bytesUsed);
	printf("Number of passes: %d\n", table->numPasses);
	printf("'correct' opcodes: %s\n", pickYN(table->corrections));
	printf("'syllable' opcodes: %s\n", pickYN(table->syllables));
	printf("'capsnocont' opcode: %s\n", pickYN(table->capsNoCont));
	printf("Hyphenation table: %s\n", pickYN(table->hyphenStatesArray));
	printf("noletsignbefore %s\n",
			print_chars(&table->noLetsignBefore[0], table->noLetsignBeforeCount));
	printf("noletsign %s\n", print_chars(&table->noLetsign[0], table->noLetsignCount));
	printf("noletsignafter %s\n",
			print_chars(&table->noLetsignAfter[0], table->noLetsignAfterCount));
	return 1;
}

static int
show_charMap(int startHash) {
	int k;
	CharDotsMapping *thisChar;
	TranslationTableOffset nextChar;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (displayTable->charToDots[k]) {
			printf("Hash=%d\n", k);
			nextChar = displayTable->charToDots[k];
			while (nextChar) {
				thisChar = (CharDotsMapping *)&displayTable->ruleArea[nextChar];
				printf("Char: %s ", print_chars(&thisChar->lookFor, 1));
				printf("dots=%s\n", _lou_showDots(&thisChar->found, 1));
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextChar = thisChar->next;
			}
		}
	return 1;
}

static int
show_dotsMap(int startHash) {
	int k;
	CharDotsMapping *thisDots;
	TranslationTableOffset nextDots;
	printf("Press enter for next or (e)xit, next-(h)ash, then enter\n");
	if (startHash < 0)
		k = 0;
	else
		k = startHash;
	for (; k < HASHNUM; k++)
		if (displayTable->dotsToChar[k]) {
			printf("Hash=%d\n", k);
			nextDots = displayTable->dotsToChar[k];
			while (nextDots) {
				thisDots = (CharDotsMapping *)&displayTable->ruleArea[nextDots];
				printf("Dots: %s ", _lou_showDots(&thisDots->lookFor, 1));
				printf("char=%s\n", print_chars(&thisDots->found, 1));
				printf("=> ");
				getInput();
				if (*inputBuffer == 'h') break;
				if (*inputBuffer == 'e') return 1;
				nextDots = thisDots->next;
			}
		}
	return 1;
}

static void
part_paramLetters(void) {
	printf("show particular hash chains.\n");
	printf("show-(f)orward-rules, show-(b)ackward-rules, show-(c)haracters, \n");
	printf("show-(d)ot-patterns, show-(C)har-to-dots, show-(D)ots-tochar\n");
	printf("(h)elp, e(x)it\n");
}

static void
particularHelp(void) {
	part_paramLetters();
}

static int
particular(void) {
	int startHash;
	widechar parsed[BUFSIZE];
	part_paramLetters();
	do {
		printf("particular: ");
		getInput();
		switch (inputBuffer[0]) {
		case 0:
			break;
		case 'h':
			particularHelp();
			break;
		case 'c':
			printf("-> ");
			getInput();
			if (!_lou_extParseChars(inputBuffer, parsed)) break;
			startHash = _lou_charHash(*parsed);
			if (table->characters[startHash] == 0) {
				printf("Character not in table.\n");
				break;
			}
			show_characters(startHash);
			break;
		case 'd':
			printf("-> ");
			getInput();
			if (!_lou_extParseDots(inputBuffer, parsed)) break;
			startHash = _lou_charHash(*parsed);
			if (table->dots[startHash] == 0) {
				printf("Dot pattern not in table.\n");
				break;
			}
			show_dots(startHash);
			break;
		case 'C':
			printf("-> ");
			getInput();
			if (!_lou_extParseChars(inputBuffer, parsed)) break;
			startHash = _lou_charHash(*parsed);
			if (displayTable->charToDots[startHash] == 0) {
				printf("Character not in table.\n");
				break;
			}
			show_charMap(startHash);
			break;
		case 'D':
			printf("-> ");
			getInput();
			if (!_lou_extParseDots(inputBuffer, parsed)) break;
			startHash = _lou_charHash(*parsed);
			if (displayTable->dotsToChar[startHash] == 0) {
				printf("Dot pattern not in table.\n");
				break;
			}
			show_dotsMap(startHash);
			break;
		case 'f':
			printf("-> ");
			getInput();
			if (!_lou_extParseChars(inputBuffer, parsed)) break;
			startHash = _lou_stringHash(parsed, 0, NULL);
			if (table->forRules[startHash] == 0) {
				printf("Character string not in table.\n");
				break;
			}
			show_forRules(startHash);
			break;
		case 'b':
			printf("-> ");
			getInput();
			if (!_lou_extParseDots(inputBuffer, parsed)) break;
			startHash = _lou_stringHash(parsed, 0, NULL);
			if (table->backRules[startHash] == 0) {
				printf("Dot pattern not in table.\n");
				break;
			}
			show_backRules(startHash);
			break;
		case 'x':
			return 1;
		default:
			printf("Bad choice.\n");
			break;
		}
	} while (inputBuffer[0] != 'x');
	return 1;
}

static void
paramLetters(void) {
	printf("Press one of the letters in parentheses, then enter.\n");
	printf("show-(f)orward-rules, show-(b)ackward-rules, show-(c)haracters, \n");
	printf("show-(d)ot-patterns, show-(C)har-to-dots, show-(D)ots-tochar\n");
	printf("show-(m)isc, show-braille(i)ndicators, show-(p)articulars\n");
	printf("(h)elp, (q)uit\n");
}

static void
commandHelp(void) {
	paramLetters();
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
		case 'h':
			commandHelp();
			break;
		case 'C':
			show_charMap(-1);
			break;
		case 'D':
			show_dotsMap(-1);
			break;
		case 'c':
			show_characters(-1);
			break;
		case 'd':
			show_dots(-1);
			break;
		case 'f':
			show_forRules(-1);
			break;
		case 'b':
			show_backRules(-1);
			break;
		case 'i':
			show_brailleIndicators();
			break;
		case 'm':
			show_misc();
			break;
		case 'p':
			particular();
			break;
		case 'q':
			return 1;
		default:
			printf("Bad choice.\n");
			break;
		}
	} while (inputBuffer[0] != 'q');
	return 1;
}

int
main(int argc, char **argv) {
	int optc;

	set_program_name(argv[0]);

	while ((optc = getopt_long(argc, argv, "hv", longopts, NULL)) != -1) switch (optc) {
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

	if (optind != argc - 1) {
		/* Print error message and exit. */
		if (optind < argc - 1)
			fprintf(stderr, "%s: extra operand: %s\n", program_name, argv[optind + 1]);
		else
			fprintf(stderr, "%s: no table specified\n", program_name);
		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	}

	_lou_getTable(argv[optind], argv[optind], &table, &displayTable);
	if (!table) {
		lou_free();
		exit(EXIT_FAILURE);
	}
	getCommands();
	lou_free();
	exit(EXIT_SUCCESS);
}
