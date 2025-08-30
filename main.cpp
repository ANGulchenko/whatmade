
#include "os_specific.h"
#include "AnyOption/anyoption.h"
#include "Log/logger.h"
#include "xattr.h"
#include "configsingleton.h"

bool shouldFilenameBeProcessed(const std::string& filename)
{
	// Check if the filename is to be checked according to monitor/ignore arrays

	bool is_filename_good = false;

	// Check if this filename is in our watch-list
	for (const std::string& path_to_monitor: ConfigSingleton::instance().paths_to_monitor)
	{
		if (filename.starts_with(path_to_monitor))
		{
			is_filename_good = true;
		}
	}

	// Check if this filename is in our ignore-list
	for (const std::string& path_to_ignore: ConfigSingleton::instance().paths_to_ignore)
	{
		if (filename.starts_with(path_to_ignore))
		{
			is_filename_good = false;
		}
	}

	return is_filename_good;
}

void processFile(const std::string& filename, const std::string& process_name)
{
	if (!shouldFilenameBeProcessed(filename))
	{
		return;
	}

	XAttr::set(filename, "whatmade", process_name);
}

int main(int argc, char *argv[])
{

	if (!XAttr::isWorking())
	{
		std::println("XATTR doesn't work");
		exit(EXIT_FAILURE);
	}

	ConfigSingleton::instance();
	Logger::init("whatmade");

	AnyOption opt;
	opt.addUsage("I know what made this file");
	opt.addUsage("Parameters: ");
	opt.addUsage("");
	opt.addUsage(" -h  --help                     Print parameters list");
	opt.addUsage(" -w  --what <filename>          What created this file?");
	opt.addUsage(" -n  --non_daemonize            Run without daemonizing");
	opt.addUsage(" -s  --stop                     Stop and quit");
	opt.addUsage("");

	opt.setFlag("help", 'h');
	opt.setCommandOption("what", 'w');
	opt.setFlag("non_daemonize", 'n');
	opt.setFlag("stop", 's');

	opt.processCommandArgs(argc, argv);

	if (!opt.hasOptions())
	{
		std::signal(SIGTERM, handle_signal);
		std::signal(SIGINT,  handle_signal);
		std::signal(SIGHUP,  handle_signal);

		Imp::daemonize();
		startWatch(ConfigSingleton::instance().paths_to_monitor, processFile);
	}

	if (opt.getFlag("non_daemonize") || opt.getFlag('n'))
	{
		startWatch(ConfigSingleton::instance().paths_to_monitor, processFile);
	}

	if (opt.getFlag("stop") || opt.getFlag('s'))
	{
		Imp::stopDaemon();
		exit(EXIT_SUCCESS);
	}

	if (opt.getFlag("help") || opt.getFlag('h'))
	{
		opt.printUsage();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('w') != NULL || opt.getValue("what") != NULL)
	{
		std::string filename = std::string(opt.getValue("what"));
		auto processname = XAttr::get(filename, "whatmade");
		if (processname)
		{
			std::println("Process name: {}", processname.value());
		}
		else
		{
			std::println("Process name: <Unknown>");
		}

		exit(EXIT_SUCCESS);
	}


	return 0;
}
