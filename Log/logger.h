#pragma once
#include <syslog.h>
#include <sstream>
#include <string>
#include <print>

class Logger
{
public:
	// Call once at program start
	static void init(const char* ident)
	{
		openlog(ident, LOG_PID | LOG_CONS, LOG_DAEMON);
	}

	// Call once at program shutdown
	static void shutdown()
	{
		closelog();
	}

	template<typename... Args>
	static void info(Args&&... args) {
		log(LOG_INFO, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void warning(Args&&... args) {
		log(LOG_WARNING, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void error(Args&&... args) {
		log(LOG_ERR, std::forward<Args>(args)...);
	}

	template<typename... Args>
	static void debug(Args&&... args) {
		log(LOG_DEBUG, std::forward<Args>(args)...);
	}

private:
	template<typename... Args>
	static void log(int priority, Args&&... args) {
		std::ostringstream oss;
		(oss << ... << args); // Fold expression to concatenate all arguments
		syslog(priority, "%s", oss.str().c_str());
		std::println("{}", oss.str().c_str());
	}
};
