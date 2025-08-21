#include "Imp.h"
#include "../Log/logger.h"

#include <fstream>
#include <iostream>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>

void Imp::stopDaemon()
{
	std::ifstream pid_file(PID_FILE);
	pid_t pid;
	if (pid_file >> pid)
	{
		kill(pid, SIGTERM);
		Logger::info("SIGTERM signal sended to daemon. PID=", pid);
		unlink(PID_FILE.c_str());
	} else
	{
		Logger::error("No PID file found\n");
	}
}

void Imp::askDaemonToRereadConfig()
{
	std::ifstream pid_file(PID_FILE);
	pid_t pid;
	if (pid_file >> pid)
	{
		kill(pid, SIGUSR1);
		Logger::info("SIGUSR1 signal sended to daemon (ask for config reread). PID=", pid);
		unlink(PID_FILE.c_str());
	} else
	{
		Logger::error("No PID file found\n");
	}
}

void Imp::statusDaemon()
{
	std::ifstream pid_file(PID_FILE);
	pid_t pid;
	if (pid_file >> pid)
	{
		if (kill(pid, 0) == 0)
			std::println("Daemon is running PID= ", pid);
		else
			std::println("Daemon not running\n");
	} else
	{
		std::println("Daemon not running\n");
	}
}

void Imp::cleanup()
{
	std::ifstream pid_file(PID_FILE);
	pid_t pid = -1;
	if (pid_file >> pid)
	{
		if (pid != -1)
		{
			close(pid);
			unlink(PID_FILE.c_str());
		}
	}
}

bool Imp::already_running()
{
	pid_t pid_fd = open(PID_FILE.c_str(), O_RDWR | O_CREAT, 0644);
	if (pid_fd < 0)
	{
		Logger::error("Unable to open PID file.\n");
		return true;
	}

	// Try to get an exclusive lock
	if (int res = flock(pid_fd, LOCK_EX | LOCK_NB); res < 0)
	{
		Logger::error("Another instance is already running.\n");
		return true;
	}

	// Truncate and write PID
	ftruncate(pid_fd, 0);
	std::string pid_str = std::to_string(getpid()) + "\n";
	write(pid_fd, pid_str.c_str(), pid_str.size());

	return false;
}

void Imp::daemonize()
{
	if (already_running())
	{
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);    // Fork failed
	if (pid > 0) exit(EXIT_SUCCESS);    // Exit parent

	setsid();                           // Create new session
	umask(0);                           // Reset file mode mask
	chdir("/");                         // Optional: change working dir

	// Second fork to prevent reacquiring a terminal
	pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);
	if (pid > 0) exit(EXIT_SUCCESS);

	// Close standard file descriptors
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Write PID file
	std::ofstream pid_file(PID_FILE);
	pid_file << getpid() << "\n";
	pid_file.close();
}
