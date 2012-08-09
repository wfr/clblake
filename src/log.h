#pragma once

enum LOG_LEVELS {
	ERROR, INFO, DEBUG
};

extern int logger_level;

void logger(int level, const char *msg);

void loggerf(int level, const char *format, ...);
