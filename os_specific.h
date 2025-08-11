#pragma once

#include <print>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/fanotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <limits.h>
#include <filesystem>

#include "Imp/Imp.h"


bool is_recent_creation(int fd)
{
	struct stat st;
	if (fstat(fd, &st) == -1)
		return false;

	time_t now = time(nullptr);
	return (now - st.st_ctime) < 2;  // Created within last 2 seconds
}

std::string get_mount_point(const std::string &path) {
	char resolved[PATH_MAX];
	if (!realpath(path.c_str(), resolved)) {
		perror("realpath");
		return "/";
	}
	std::string abs_path = resolved;

	std::ifstream file("/proc/self/mountinfo");
	if (!file.is_open()) {
		perror("open mountinfo");
		return "/";
	}

	std::string best_match;
	size_t best_len = 0;

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string discard;
		std::string mount_point;

		// mountinfo format:
		// ID parent major:minor root mount_point options ...
		// We need the 5th field (mount_point)
		for (int field = 1; field <= 5; ++field) {
			if (!(iss >> discard)) break;
			if (field == 5) {
				mount_point = discard;
			}
		}

		// Find the longest mount point that is a prefix of abs_path
		if (abs_path.rfind(mount_point, 0) == 0) {
			size_t len = mount_point.size();
			if (len > best_len) {
				best_len = len;
				best_match = mount_point;
			}
		}
	}

	return best_match.empty() ? "/" : best_match;
}



std::optional<std::string>try_get_process_cmdline(pid_t pid)
{
	// Try /proc/[pid]/cmdline
	{
		std::stringstream path;
		path << "/proc/" << pid << "/cmdline";
		std::ifstream file(path.str(), std::ios::in | std::ios::binary);
		if (file)
		{
			std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (!data.empty())
			{
				for (char &c : data)
				{
					if (c == '\0') c = ' ';
				}
				return data;
			}
		}
	}

	// Try /proc/[pid]/exe (symlink to the binary)
	{
		char link_path[64];
		snprintf(link_path, sizeof(link_path), "/proc/%d/exe", pid);

		char exe_path[PATH_MAX];
		ssize_t len = readlink(link_path, exe_path, sizeof(exe_path) - 1);
		if (len > 0)
		{
			exe_path[len] = '\0';
			return std::string(exe_path);
		}
	}

	// Try /proc/[pid]/comm
	// It is truncated to 16 chars. Better than nothing though.
	{
		std::stringstream path;
		path << "/proc/" << pid << "/comm";
		std::ifstream file(path.str());
		if (file)
		{
			std::string line;
			if (std::getline(file, line))
			{
				return line;
			}
		}
	}

	return std::nullopt;
}

int fanotify_setup(const std::set<std::string>& paths_to_control)
{
	std::set<std::string>mount_points;
	int fan_fd;

	for(const std::string& path: paths_to_control)
	{
		mount_points.insert(get_mount_point(path));
	}

	fan_fd = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY | O_CLOEXEC);
	if (fan_fd < 0)
	{
		perror("fanotify_init: fan_fd < 0");
		return 1;
	}

	for (const std::string& mount_point: mount_points)
	{
		if (fanotify_mark(fan_fd,
						  FAN_MARK_ADD | FAN_MARK_MOUNT,
						  FAN_OPEN | FAN_ONDIR | FAN_EVENT_ON_CHILD,
						  AT_FDCWD,
						  mount_point.c_str()) < 0)
		{
			perror("fanotify_mark < 0");
			return 1;
		}
	}

	return fan_fd;
}

void handle_signal(int signum)
{
	std::cerr << "Handle signal. Cleanup()\n";
	Imp::cleanup();
	std::exit(signum);
}

std::vector<std::string> get_filenames_to_delete(const std::vector<std::string>& filenames)
{
	std::vector<std::string> result;
	for (const auto& filename: filenames)
	{
		if (!std::filesystem::exists(filename))
		{
			result.push_back(filename);
		}
	}

	return result;
}
