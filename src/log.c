#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int logger_level = INFO;

void logger(int level, const char *msg) {
	if(level > logger_level)
		return;
	switch(level) {
		case INFO:
			fprintf(stdout, "%s\n", msg);
			break;
		case ERROR:
			fprintf(stderr, "ERROR: %s\n", msg);
			break;
		case DEBUG:
			fprintf(stderr, "DEBUG: %s\n", msg);
			break;
	}
}

void loggerf(int level, const char *fmt, ...) {
	int n;
	int size = 100;
	char *p, *np;
	va_list ap;

	if(level > logger_level)
		return;

	if ((p = malloc(size)) == NULL)
		return;

	while (1) {
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);

		if (n > -1 && n < size) {
			logger(level, p);
			free(p);
			return; 
		}

		if (n > -1) /* glibc 2.1 and later */
			size = n+1;

		if ((np = realloc (p, size)) == NULL) {
			free(p);
			return;
		} else {
			p = np;
		}
	}
}

