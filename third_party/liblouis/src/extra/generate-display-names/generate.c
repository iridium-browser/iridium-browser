#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"
#include "displayLanguage.h"

static const char *
displayLanguage(const char *lang) {
	return DisplayLanguage((char *)lang);
}

static const char *
getDisplayName(const char *table) {
	return lou_getTableInfo(table, "display-name");
}

static char *
generateDisplayName(const char *table) {
	char *name;
	char *language;
	char *locale;
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
	locale = lou_getTableInfo(table, "locale");
	if (!locale)
		return NULL;
	language = displayLanguage(locale);
	n += sprintf(n, "%s", language);
	q += sprintf(q, "locale:%s", locale);
	free(locale);
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
						for (m = matches; *m; m++) free(*m);
						free(matches);
					}
					q = q_save;
				}
				q += sprintf(q, " dots:%s", dots);
				free(dots);
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
					if (!uncontracted || matches[0] && matches[1])
						otherUncontracted = 1;
					for (m = matches; *m; m++) free(*m);
					free(matches);
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
						free(*m);
					}
					free(matches);
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
					for (m = matches; *m; m++) free(*m);
					free(matches);
				}
				q = q_save;
				q += sprintf(q, " contraction:%s", contraction);
				free(contraction);
			}
			dots = lou_getTableInfo(table, "dots");
			if (dots) {
				int otherDots = 0;
				matches = lou_findTables(query);
				if (matches) {
					for (m = matches; *m; m++) {
						if (!otherDots) {
							char *d = lou_getTableInfo(*m, "dots");
							if (d && strcmp(dots, d))
								otherDots = 1;
						}
						free(*m);
					}
				}
				if (otherDots)
					n += sprintf(n, " %s-dot", dots);
				free(dots);
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
			free(grade);
		}
		free(type);
	}
	n += sprintf(n, " braille");
	version = lou_getTableInfo(table, "version");
	if (version) {
		n += sprintf(n, " (%s standard)", version);
		free(version);
	}
	return name;
}

int main(int argc, char **argv) {
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
	while (getline(&line, &len, fp) != -1) {
		char *cp = line;
		int generate = 0;
		if (*cp == '*') {
			generate = 1;
			cp++;
		}
		while (*cp && *cp == ' ')
			cp++;
		if (*cp == '\n' || *cp == '#') {
			if (!generate)
				continue;
			else
				goto parse_error;
		} else if (*cp) {
			char *table = cp;
			cp++;
			while (*cp && *cp != ' ' && *cp != '\n' && *cp != '#')
				cp++;
			if (*cp == ' ') {
				cp++;
				while (*cp && *cp == ' ')
					cp++;
				if (*cp && *cp != '\n' && *cp != '#') {
					char *expectedName = cp;
					cp++;
					while (*cp && *cp != '\n' && *cp != '#')
						cp++;
					if (*cp) {
						cp--;
						while (*cp == ' ')
							cp--;
						cp++;
						*cp = '\0';
						cp = table;
						while (*cp != ' ')
							cp++;
						*cp = '\0';
						const char *actualName = getDisplayName(table);
						if (!actualName) {
							fprintf(stderr, "No display-name field in table %s\n", table);
							result = 1;
						} else {
							if (strcmp(actualName, expectedName) != 0) {
								fprintf(stderr, "%s: %s != %s\n", table, actualName, expectedName);
								fprintf(stderr, "   cat %s | sed 's/^\\(#-display-name: *\\).*$/\\1%s/g' > %s.tmp\n", table, expectedName, table);
								fprintf(stderr, "   mv %s.tmp %s\n", table, table);
								result = 1;
							}
							const char *generatedName = generateDisplayName(table);
							if (!generatedName || !*generatedName) {
								if (generate) {
									fprintf(stderr, "No display-name could be generated for table %s\n", table);
									result = 1;
								}
							} else if (strcmp(actualName, generatedName) != 0) {
								if (generate) {
									fprintf(stderr, "%s: %s != %s\n", table, actualName, generatedName);
									result = 1;
								}
							} else {
								if (!generate) {
									fprintf(stderr, "%s: %s == %s\n", table, actualName, generatedName);
									result = 1;
								}
							}
						}
						continue;
					}
				}
			}
		}
	  parse_error:
		fprintf(stderr, "Could not parse line: %s\n", line);
		exit(EXIT_FAILURE);
	}
	free(line);
	return result;
}
