#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "internal.h"
#include "displayLanguage.h"

static const char *
displayLanguage(const char *lang) {
	return DisplayLanguage((char *)lang);
}

static const char *
displayRegion(const char *region) {
	return DisplayRegion((char *)region);
}

static char *
generateDisplayName(const char *table) {
	char *name;
	char *language;
	char *region;
	char *type;
	char *dots;
	char *contraction;
	char *grade;
	char *version;
	char *query;
	char **matches;
	char *n;
	char *q;
	char **m;
	name = (char *)malloc(100 * sizeof(*name));
	n = name;
	query = (char *)malloc(100 * sizeof(*query));
	q = query;
	language = lou_getTableInfo(table, "language");
	if (!language) return NULL;
	n += sprintf(n, "%s", displayLanguage(language));
	q += sprintf(q, "language:%s", language);
	region = lou_getTableInfo(table, "region");
	if (region) q += sprintf(q, " region:%s", region);
	type = lou_getTableInfo(table, "type");
	if (type) {
		q += sprintf(q, " type:%s", type);
		if (!strcmp(type, "computer")) {
			dots = lou_getTableInfo(table, "dots");
			if (dots) {
				if (!strcmp(dots, "6")) {
					n += sprintf(n, " %s-dot", dots);
				} else {
					char *q_save = q;
					q += sprintf(q, " dots:6");
					matches = lou_findTables(query);
					if (matches) {
						n += sprintf(n, " %s-dot", dots);
						// for (m = matches; *m; m++) free(*m);
						// free(matches);
					}
					q = q_save;
				}
				q += sprintf(q, " dots:%s", dots);
				// free(dots);
			}
			n += sprintf(n, " %s", type);
		} else if (!strcmp(type, "literary")) {
			int uncontracted = 0;
			int fullyContracted = 0;
			int partiallyContracted = 0;
			int otherUncontracted = 0;
			int otherFullyContracted = 0;
			int otherPartiallyContracted = 0;
			int twoOrMorePartiallyContracted = 0;
			contraction = lou_getTableInfo(table, "contraction");
			if (contraction) {
				char *q_save = q;
				uncontracted = !strcmp(contraction, "no");
				fullyContracted = !strcmp(contraction, "full");
				partiallyContracted = !strcmp(contraction, "partial");
				otherUncontracted = 0;
				q += sprintf(q, " contraction:no");
				matches = lou_findTables(query);
				if (matches) {
					if (!uncontracted || matches[0] && matches[1]) otherUncontracted = 1;
					// for (m = matches; *m; m++) free(*m);
					// free(matches);
				}
				q = q_save;
				otherPartiallyContracted = 0;
				twoOrMorePartiallyContracted = 0;
				grade = NULL;
				q += sprintf(q, " contraction:partial");
				matches = lou_findTables(query);
				if (matches) {
					for (m = matches; *m; m++) {
						if (!twoOrMorePartiallyContracted) {
							char *g = lou_getTableInfo(*m, "grade");
							if (g) {
								if (!grade)
									grade = g;
								else if (strcmp(grade, g))
									twoOrMorePartiallyContracted = 1;
							}
						}
						// free(*m);
					}
					// free(matches);
					if (!partiallyContracted || twoOrMorePartiallyContracted)
						otherPartiallyContracted = 1;
					if (twoOrMorePartiallyContracted)
						grade = lou_getTableInfo(table, "grade");
					else
						grade = NULL;
				}
				q = q_save;
				otherFullyContracted = 0;
				q += sprintf(q, " contraction:full");
				matches = lou_findTables(query);
				if (matches) {
					if (!fullyContracted || matches[0] && matches[1])
						otherFullyContracted = 1;
					// for (m = matches; *m; m++) free(*m);
					// free(matches);
				}
				q = q_save;
				q += sprintf(q, " contraction:%s", contraction);
				// free(contraction);
			}
			dots = lou_getTableInfo(table, "dots");
			if (dots) {
				int otherDots = 0;
				matches = lou_findTables(query);
				if (matches) {
					for (m = matches; *m; m++) {
						if (!otherDots) {
							char *d = lou_getTableInfo(*m, "dots");
							if (d && strcmp(dots, d)) otherDots = 1;
						}
						// free(*m);
					}
					// free(matches);
				}
				if (otherDots) n += sprintf(n, " %s-dot", dots);
				// free(dots);
			}
			if (uncontracted) {
				if (otherFullyContracted || otherPartiallyContracted)
					n += sprintf(n, " uncontracted");
			} else if (fullyContracted) {
				if (otherPartiallyContracted) {
					if (twoOrMorePartiallyContracted && grade)
						n += sprintf(n, " grade %s contracted", grade);
					else
						n += sprintf(n, " contracted");
				} else if (otherUncontracted) {
					n += sprintf(n, " contracted");
				}
			} else if (partiallyContracted) {
				if (twoOrMorePartiallyContracted && grade)
					n += sprintf(n, " grade %s contracted", grade);
				else
					n += sprintf(n, " partially contracted");
			}
			// free(grade);
		}
		// free(type);
	}
	n += sprintf(n, " braille");
	if (region && strlen(region) > strlen(language) &&
			!strncmp(language, region, strlen(language)) &&
			region[strlen(language)] == '-') {
		char *r = displayRegion(&region[strlen(language) + 1]);
		if (r && *r) n += sprintf(n, " as used in %s", r);
	}
	// free(region);
	// free(language);
	version = lou_getTableInfo(table, "version");
	if (version) {
		matches = lou_findTables(query);
		if (matches) {
			if (matches[0] && matches[1]) n += sprintf(n, " (%s standard)", version);
			// free(matches);
		}
		// free(version);
	}
	return name;
}

int
main(int argc, char **argv) {
	int COLUMN_INDEX_NAME = -1;
	int COLUMN_DISPLAY_NAME = -1;
	int result = 0;
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	if (argc != 2) {
		fprintf(stderr, "One argument expected\n");
		exit(EXIT_FAILURE);
	}
	fp = fopen(argv[1], "rb");
	if (!fp) {
		fprintf(stderr, "Could not open file: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	lou_setLogLevel(LOU_LOG_WARN);
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fprintf(stderr, "Unexpected error\n");
		exit(EXIT_FAILURE);
	}
	int cwdLen = strlen(cwd);
	char *tablePath = _lou_getTablePath();
	if (strncmp(cwd, tablePath, cwdLen) || tablePath[cwdLen] != '/') {
		fprintf(stderr, "Unexpected table path: %s\n", tablePath);
		exit(EXIT_FAILURE);
	}
	int tablePathLen = strlen(tablePath);
	char **tables = lou_listTables();
	int tableCount = 0;
	for (char **t = tables; *t; t++) {
		tableCount++;
		if (strncmp(tablePath, *t, tablePathLen) || (*t)[tablePathLen] != '/') {
			fprintf(stderr, "Unexpected table location: %s\n", *t);
			exit(EXIT_FAILURE);
		}
	}
	free(tablePath);
	while (getline(&line, &len, fp) != -1) {
		char *cp = line;
		int generate = 0;
		if (*cp == '*') {
			generate = 1;
			cp++;
		}
		while (*cp == ' ') cp++;
		if (*cp == '\0' || *cp == '\n' || *cp == '#') {
			if (!generate)
				continue;
			else
				goto parse_error;
		}
		char *table = cp;
		cp++;
		while (*cp != ' ' && *cp != '\0' && *cp != '\n' && *cp != '#') cp++;
		if (*cp != ' ') goto parse_error;
		*cp = '\0';
		cp++;
		while (*cp == ' ') cp++;
		if (*cp == '\0' || *cp == '\n' || *cp == '#') goto parse_error;
		char *expectedIndexName = cp;
		if (COLUMN_INDEX_NAME < 0)
			COLUMN_INDEX_NAME = expectedIndexName - line;
		else if (expectedIndexName != line + COLUMN_INDEX_NAME)
			goto parse_error;
		while (*cp != '\0' && *cp != '\n' && *cp != '#') {
			if (*cp == ' ' && cp[1] == ' ') {
				*cp = '\0';
				cp++;
				break;
			}
			cp++;
		}
		if (*cp != ' ') goto parse_error;
		while (*cp == ' ') cp++;
		if (*cp == '\0' || *cp == '\n' || *cp == '#') goto parse_error;
		char *expectedDisplayName = cp;
		if (COLUMN_DISPLAY_NAME < 0)
			COLUMN_DISPLAY_NAME = expectedDisplayName - line;
		else if (expectedDisplayName != line + COLUMN_DISPLAY_NAME)
			goto parse_error;
		while (*cp != '\0' && *cp != '\n' && *cp != '#') {
			if (*cp == ' ' && cp[1] == ' ') break;
			cp++;
		}
		*cp = '\0';
		int found = 0;
		for (int k = 0; k < tableCount; k++) {
			if (tables[k]) {
				if (!strcmp(&tables[k][cwdLen + 1], table)) {
					tables[k] = NULL;
					found = 1;
					break;
				}
			}
		}
		if (!found) {
			fprintf(stderr, "Table not in table path: %s\n", table);
			result = 1;
		}
		const char *actualIndexName = lou_getTableInfo(table, "index-name");
		if (!actualIndexName) {
			fprintf(stderr, "No index-name field in table %s\n", table);
			result = 1;
		} else {
			if (strcmp(actualIndexName, expectedIndexName) != 0) {
				fprintf(stderr, "%s: %s != %s\n", table, actualIndexName,
						expectedIndexName);
				fprintf(stderr,
						"   cat %s | sed 's/^\\(#-index-name: *\\).*$/\\1%s/g' > "
						"%s.tmp\n",
						table, expectedIndexName, table);
				fprintf(stderr, "   mv %s.tmp %s\n", table, table);
				result = 1;
			}
		}
		const char *actualDisplayName = lou_getTableInfo(table, "display-name");
		if (!actualDisplayName) {
			fprintf(stderr, "No display-name field in table %s\n", table);
			result = 1;
		} else {
			if (strcmp(actualDisplayName, expectedDisplayName) != 0) {
				fprintf(stderr, "%s: %s != %s\n", table, actualDisplayName,
						expectedDisplayName);
				fprintf(stderr,
						"   cat %s | sed 's/^\\(#-display-name: *\\).*$/\\1%s/g' > "
						"%s.tmp\n",
						table, expectedDisplayName, table);
				fprintf(stderr, "   mv %s.tmp %s\n", table, table);
				result = 1;
			}
			const char *generatedDisplayName = generateDisplayName(table);
			if (!generatedDisplayName || !*generatedDisplayName) {
				if (generate) {
					fprintf(stderr, "No display-name could be generated for table %s\n",
							table);
					result = 1;
				}
			} else if (strcmp(actualDisplayName, generatedDisplayName) != 0) {
				if (generate) {
					fprintf(stderr, "%s: %s != %s\n", table, actualDisplayName,
							generatedDisplayName);
					result = 1;
				}
			} else {
				if (!generate) {
					fprintf(stderr, "%s: %s == %s\n", table, actualDisplayName,
							generatedDisplayName);
					result = 1;
				}
			}
		}
		continue;
	parse_error:
		fprintf(stderr, "Could not parse line: %s\n", line);
		exit(EXIT_FAILURE);
	}
	free(line);
	for (int k = 0; k < tableCount; k++) {
		if (tables[k]) {
			fprintf(stderr, "Table not in list: %s\n", tables[k]);
			result = 1;
			free(tables[k]);
		}
	}
	free(tables);
	return result;
}
