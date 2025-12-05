#ifndef OS_SPECIFIC_H
#define OS_SPECIFIC_H

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
#include <filesystem>
#include <functional>

#include "Imp/Imp.h"

bool is_recent_creation_statx(const std::string& filename)
{
	struct statx stx;

	statx(AT_FDCWD, filename.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BASIC_STATS | STATX_BTIME, &stx);
	if (stx.stx_mask & STATX_BTIME)
	{
		time_t now = time(nullptr);
		return (now - stx.stx_btime.tv_sec) < 2;
	}

	return false;
}


/*bool is_recent_creation(int fd)
{
	struct stat st;

	if (fstat(fd, &st) == -1)
		return false;

	time_t now = time(nullptr);
	return (now - st.st_ctime) < 2;  // Created within last 2 seconds
}*/

std::set<std::string> get_mount_point(const std::string &path)
{
	std::set<std::string> result;
	std::set<std::string> all_mounting_points;

	char resolved[PATH_MAX];
	if (!realpath(path.c_str(), resolved))
	{
		perror("realpath");
		result.insert("/");
		return result;
	}
	std::string abs_path = resolved;

	std::ifstream file("/proc/self/mountinfo");
	if (!file.is_open())
	{
		perror("open mountinfo");
		result.insert("/");
		return result;
	}

	std::string best_match;
	size_t best_len = 0;

	std::string line;
	while (std::getline(file, line))
	{
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

		all_mounting_points.insert(mount_point);
	}

	// Looking for the mount point of the user dir
	for (auto& mount_point: all_mounting_points)
	{
		if (abs_path.rfind(mount_point, 0) == 0)
		{
			size_t len = mount_point.size();
			if (len > best_len)
			{
				best_len = len;
				best_match = mount_point;
			}
		}
	}

	result.insert(best_match);

	// Looking for the mount sub points
	for (auto& mount_point: all_mounting_points)
	{
		if (mount_point.find(abs_path) == 0)
		{
			result.insert(mount_point);
		}
	}

	return result;
}



std::optional<std::string>try_get_process_cmdline(pid_t pid)
{
	// Try /proc/[pid]/cmdline
	{
		std::string path = std::format("/proc/{}/cmdline", pid);
		std::ifstream file(path, std::ios::in | std::ios::binary);
		if (file)
		{
			std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (!data.empty())
			{
				//std::replace(data.begin(), data.end(), '\0', ' ');
				return data;
			}
		}
	}

	// Try /proc/[pid]/exe (symlink to the binary)
	{
		std::string link_path = std::format("/proc/{}/exe", pid);
		std::error_code ec;

		std::filesystem::path exe_path = std::filesystem::read_symlink(link_path, ec);
		if (!ec)
		{
			return exe_path.string();
		}

	}

	// Try /proc/[pid]/comm
	// It is truncated to 16 chars. Better than nothing though.
	{
		std::string path = std::format("/proc/{}/comm", pid);
		std::ifstream file(path);
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

std::optional<std::string> filename_from_fd(int fd)
{
	std::filesystem::path link_path = std::format("/proc/self/fd/{}", fd);
	std::error_code ec;

	std::filesystem::path file_path = std::filesystem::read_symlink(link_path, ec);
	if (!ec)
	{
		return file_path.string();
	}

	return std::nullopt;
}

int fanotify_setup(const std::set<std::string>& paths_to_control)
{
	std::set<std::string>mount_points;

	for(const std::string& path: paths_to_control)
	{
		std::set<std::string> tmp_mnt_points = get_mount_point(path);
		mount_points.insert(tmp_mnt_points.begin(), tmp_mnt_points.end());
	}

	int fan_fd = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY | O_CLOEXEC);
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

void startWatch(const std::set<std::string>& paths_to_monitor,
				std::function<void(const std::string& filename, const std::string& process_name)> file_processor)
{

	int fan_fd = fanotify_setup(paths_to_monitor);

	char buffer[4096];
	while (true)
	{
		ssize_t len = read(fan_fd, buffer, sizeof(buffer));
		if (len <= 0)
		{
			perror("read");
			continue;
		}

		struct fanotify_event_metadata* metadata = reinterpret_cast<fanotify_event_metadata*>(buffer);

		while (FAN_EVENT_OK(metadata, len))
		{
			if (metadata->fd >= 0 && metadata->mask & FAN_OPEN)
			{
				std::optional<std::string> app_name = try_get_process_cmdline(metadata->pid);
				std::optional<std::string> file_name = filename_from_fd(metadata->fd);

				if (app_name && file_name)
				{
					if (is_recent_creation_statx(file_name.value()))
					{
						file_processor(file_name.value(), app_name.value());
					}
				}
				close(metadata->fd);
			}
			metadata = FAN_EVENT_NEXT(metadata, len);
		}
	}

	close(fan_fd);
}

std::optional<std::vector<std::string>> getAllFilenamesFromDir(std::string dirname)
{
	if (!std::filesystem::is_directory(dirname))
	{
		return std::nullopt;
	}

	std::vector<std::string> result;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(dirname))
	{
		if (std::filesystem::is_regular_file(entry.path()))
		{
			result.push_back(entry.path());
		}
	}

	return result;
}

uintmax_t getFileSize(const std::string& filename)
{
	return std::filesystem::file_size(filename);
}

bool isDir(const std::string& name)
{
	return std::filesystem::is_directory(name);
}


#endif
