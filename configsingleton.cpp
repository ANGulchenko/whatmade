#include <filesystem>
#include <fstream>
//#include <iostream>
#include <print>
#include <libconfig.h++>

#include "configsingleton.h"


ConfigSingleton::ConfigSingleton()
{
	if (std::filesystem::exists(std::filesystem::path(config_file)))
	{
		libconfig::Config cfg;
		try
		{
			cfg.readFile(config_file.c_str());
		}
		catch(const libconfig::FileIOException &fioex)
		{
			std::println("I/O error while reading configuration file.");
			exit(EXIT_FAILURE);
		}
		catch(const libconfig::ParseException &pex)
		{
			std::println("Parse error at {}:{} - {}", pex.getFile(), pex.getLine(), pex.getError());
			exit(EXIT_FAILURE);
		}

		const libconfig::Setting& root = cfg.getRoot();

		const libconfig::Setting& monitor_root = root["monitor"];
		const libconfig::Setting& ignore_root = root["ignore"];

		for (int i = 0; i < monitor_root.getLength(); ++i)
		{
			const char* path = monitor_root[i];
			std::string path_str{path};
			paths_to_monitor.insert(path_str);
			//std::println("MONITOR {}", path_str);
		}

		for (int i = 0; i < ignore_root.getLength(); ++i)
		{
			const char* path = ignore_root[i];
			std::string path_str{path};
			paths_to_ignore.insert(path_str);
			//std::println("IGNORE {}", path_str);
		}

	}else
	{
		const char* default_config =
		R"CONFIG(
monitor = (
	"/home/astahl/.config",
	"/home/astahl/.cache"
);

ignore = (
	"/home/astahl/.cache/mozilla"
);
		)CONFIG";

		std::filesystem::path path = config_file;
		std::filesystem::create_directories(path.parent_path());

		std::ofstream ofs(path);
		ofs << default_config;
		ofs.close();

		std::println("No config was detected. Created a new one: {}. Please edit it and start the daemon again.", config_file);
		exit(EXIT_FAILURE);
	}
}
