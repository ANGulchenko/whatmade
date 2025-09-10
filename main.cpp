
#include <map>

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

std::vector<std::string> splitRawProcessLine(const std::string& raw)
{
	std::stringstream process(raw);
	std::string segment;
	std::vector<std::string> seglist;

	while(std::getline(process, segment, '\0'))
	{
	   seglist.push_back(segment);
	}

	return seglist;
}

std::string getStrProcessNameFromFilename(const std::string& filename)
{
	auto processname = XAttr::get(filename, "whatmade");
	if (!processname)
	{
		return "<Unknown>";
	}

	std::vector<std::string> seglist = splitRawProcessLine(processname.value());
	if (seglist.size() > 0)
	{
		return seglist[0];
	}else
	{
		return "<Unknown>";
	}

}

void printProcessData(const std::string& filename, bool raw)
{
	auto processname = XAttr::get(filename, "whatmade");
	if (!processname)
	{
		std::println("Process name: <Unknown>");
	}else
	{
		if (raw)
		{
			std::println("{}", processname.value());
		}else
		{
			std::vector<std::string> seglist = splitRawProcessLine(processname.value());
			if (seglist.size() > 0)
			{
				std::println("Process:\n{}", seglist[0]);
				if (seglist.size() > 1)
				{
					std::println("Parameters:");
					for (size_t i = 1; i < seglist.size(); i++)
					{
						std::println("{}", seglist[i]);
					}
				}
			}

		}
	}
}

void printDirSummary(const std::string& dirname)
{
	std::optional<std::vector<std::string>> files = getAllFilenamesFromDir(dirname);
	if (!files)
	{
		std::println("Not a directory or an empty directory");
		return;
	}

	std::map<std::string /*processname*/, std::pair<int /*number*/, uintmax_t /*total_size*/>> result;

	for (const auto& file: files.value())
	{
		std::string processname = getStrProcessNameFromFilename(file);
		uintmax_t filesize = getFileSize(file);

		result[processname].first++;
		result[processname].second += filesize;
	}

	// Print out the summary
	std::println("Dir summary (process name, number of files, total size):");

	for (const auto& [process, stats] : result)
	{
		const auto& [count, total_size] = stats;
		std::println("{}\t\t\t{}\t\t{}", process, count, total_size);
	}
}

void clearAttributes(const std::string& name)
{
	if (!isDir(name))
	{
		XAttr::remove(name, "whatmade");
	}else
	{
		std::optional<std::vector<std::string>> files = getAllFilenamesFromDir(name);
		if (files)
		{
			for (const auto& file: files.value())
			{
				XAttr::remove(file, "whatmade");
			}
		}
	}
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
	opt.addUsage(" -w  --what <filename>          What created this file?(Human-redable format)");
	opt.addUsage(" -r  --raw <filename>           What created this file?(Raw format)");
	opt.addUsage(" -d  --dir <dirname>            Directory summary");
	opt.addUsage(" -c  --clear <file/dirname>     Clears process data from file or all files in the dir");
	opt.addUsage(" -n  --non_daemonize            Run without daemonizing");
	opt.addUsage(" -s  --stop                     Stop and quit");
	opt.addUsage("");

	opt.setFlag("help", 'h');
	opt.setCommandOption("what", 'w');
	opt.setCommandOption("raw",  'r');
	opt.setCommandOption("dir",  'd');
	opt.setCommandOption("clear",'c');
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
		printProcessData(filename, false);
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('r') != NULL || opt.getValue("raw") != NULL)
	{
		std::string filename = std::string(opt.getValue("raw"));
		printProcessData(filename, true);
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('d') != NULL || opt.getValue("dir") != NULL)
	{
		std::string dirname = std::string(opt.getValue("dir"));
		printDirSummary(dirname);
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('c') != NULL || opt.getValue("clear") != NULL)
	{
		std::string name = std::string(opt.getValue("clear"));
		clearAttributes(name);
		exit(EXIT_SUCCESS);
	}


	return 0;
}
