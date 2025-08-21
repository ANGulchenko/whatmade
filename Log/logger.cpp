#include "logger.h"

void Logger::init(const char* ident)
{
	openlog(ident, LOG_PID | LOG_CONS, LOG_DAEMON);
}

void Logger::shutdown()
{
	closelog();
}
