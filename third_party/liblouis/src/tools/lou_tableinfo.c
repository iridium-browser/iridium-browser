/* liblouis Braille Translation and Back-Translation Library

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
#include <stdlib.h>
#include "liblouis.h"
#include <getopt.h>
#include "progname.h"
#include "version-etc.h"

static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ NULL, 0, NULL, 0 },
};

const char version_etc_copyright[] = "Copyright %s %d Bert Frees";

#define AUTHORS "Bert Frees"

static void
print_help(void) {
	printf("\
Usage: %s [KEY] TABLE\n",
			program_name);

	fputs("\
Print all table metadata defined in TABLE, or a specific metadata field\n\
if KEY is specified. Return 0 if the requested metadata could be found,\n\
1 otherwise.\n\n",
			stdout);

	fputs("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exi\n",
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

int
main(int argc, char **argv) {
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
	if (optind > argc - 1) {
		fprintf(stderr, "%s: no table specified\n", program_name);
		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	} else if (optind == argc - 1) {
		// const char *table = argv[optind];
		fprintf(stderr, "Not supported yet\n");
		exit(EXIT_FAILURE);
	} else if (optind == argc - 2) {
		const char *key = argv[optind];
		const char *table = argv[optind + 1];
		const char *value = lou_getTableInfo(table, key);
		if (value != NULL) {
			printf("%s\n", value);
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "%s: no such field in %s\n", key, table);
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "%s: extra operand: %s\n", program_name, argv[optind + 1]);
		fprintf(stderr, "Try `%s --help' for more information.\n", program_name);
		exit(EXIT_FAILURE);
	}
}
