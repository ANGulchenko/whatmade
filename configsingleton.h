#ifndef CONFIGSINGLETON_H
#define CONFIGSINGLETON_H

#include <set>
#include <string>

class ConfigSingleton
{
public:
	static ConfigSingleton& instance()
	{
		static ConfigSingleton instance;
		return instance;
	}

	ConfigSingleton(const ConfigSingleton&) = delete;
	ConfigSingleton& operator = (const ConfigSingleton&) = delete;

	std::set<std::string> paths_to_monitor;
	std::set<std::string> paths_to_ignore;

	const std::string config_file {"/etc/whatmade/config.cfg"};

private:

	ConfigSingleton();
	~ConfigSingleton(){};

};

#endif // CONFIGSINGLETON_H
