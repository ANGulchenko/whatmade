
#include <thread>
#include <chrono>


#include "os_specific.h"
#include "sqlite_specific.h"
#include "AnyOption/anyoption.h"
#include "Log/logger.h"

std::set<std::string> paths_to_monitor {"/home/astahl/.config"};
std::set<std::string> paths_to_ignore {"/home/astahl/.cache/mozilla"};

bool shouldFilenameBeProcessed(const std::string& filename)
{
	// Check if the filename is to be checked according to
	// the paths_to_control array.

	bool is_filename_good = false;

	// Check if this filename is in our watch-list
	for (const std::string& path_to_monitor: paths_to_monitor)
	{
		if (filename.starts_with(path_to_monitor))
		{
			is_filename_good = true;
		}
	}

	// Check if this filename is in our ignore-list
	for (const std::string& path_to_ignore: paths_to_ignore)
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

	DB::add_filename_process_pair(filename, process_name);
}



void handle_update_signal(int /*signum*/)
{
	std::cerr << "Handle signal. Update()\n";
	DB::get_directories(paths_to_monitor, paths_to_ignore);
}

void run_cleanup()
{
	std::vector<std::string> all_filenames = DB::get_all_filenames();
	std::vector<std::string> nonexistent_filenames = get_filenames_to_delete(all_filenames);
	DB::remove_these_filenames(nonexistent_filenames);
}

int main(int argc, char *argv[])
{
	Logger::init("whomade");
	DB::connect();

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	AnyOption opt;
	opt.addUsage("I know who made this file");
	opt.addUsage("Parameters: ");
	opt.addUsage("");
	opt.addUsage(" -h  --help                     Print parameters list");
	opt.addUsage(" -w  --who <filename>           Who created this file?");
	opt.addUsage(" -a  --add <dir>                Add dir to monitor");
	opt.addUsage(" -r  --remove <dir>             Remove dir from monitor list");
	opt.addUsage(" -i  --ignore <dir>             Add dir to ignore");
	opt.addUsage(" -p  --pay_attention <dir>      Remove dir from ignore list");
	opt.addUsage(" -l  --lists                    Print monitor/ignore lists");
	opt.addUsage(" -n  --non_daemonize            Run without daemonizing");
	opt.addUsage(" -s  --stop                     Stop and quit");
	opt.addUsage(" -c  --clean                    DB cleanup");
	opt.addUsage("");

	opt.setFlag("help", 'h');
	opt.setCommandOption("who", 'w');
	opt.setCommandOption("add", 'a');
	opt.setCommandOption("remove", 'r');
	opt.setCommandOption("ignore", 'i');
	opt.setCommandOption("pay_attention", 'p');
	opt.setFlag("lists", 'l');
	opt.setFlag("non_daemonize", 'n');
	opt.setFlag("stop", 's');
	opt.setFlag("clean", 'c');

	opt.processCommandArgs(argc, argv);

	if (!opt.hasOptions())
	{
		Imp::daemonize();
		signal(SIGUSR1, handle_update_signal);

		std::thread cleanup_thread(
			[]()
			{
				while (true)
				{
					std::this_thread::sleep_for(std::chrono::hours(1));
					run_cleanup();
				}
			}
		);

		cleanup_thread.detach();
		DB::get_directories(paths_to_monitor, paths_to_ignore);
		startWatch(paths_to_monitor, paths_to_ignore, processFile);
	}

	if (opt.getFlag("non_daemonize") || opt.getFlag('n'))
	{
		signal(SIGUSR1, handle_update_signal);

		std::thread cleanup_thread(
			[]()
			{
				while (true)
				{
					std::this_thread::sleep_for(std::chrono::hours(1));
					run_cleanup();
				}
			}
		);

		cleanup_thread.detach();
		DB::get_directories(paths_to_monitor, paths_to_ignore);
		startWatch(paths_to_monitor, paths_to_ignore, processFile);
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

	if (opt.getFlag("clean") || opt.getFlag('c'))
	{
		run_cleanup();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('w') != NULL || opt.getValue("who") != NULL)
	{
		std::string filename = std::string(opt.getValue("who"));
		std::string processname = DB::get_process_by_filename(filename);
		std::println("Process name: {}", processname);
		//std::fflush(stdout);
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('a') != NULL || opt.getValue("add") != NULL)
	{
		std::string dir = std::string(opt.getValue("add"));
		DB::addDir(DB::DirAction::ADD, DB::DirType::MONITOR, dir);
		Imp::askDaemonToRereadConfig();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('r') != NULL || opt.getValue("remove") != NULL)
	{
		std::string dir = std::string(opt.getValue("remove"));
		DB::addDir(DB::DirAction::REMOVE, DB::DirType::MONITOR, dir);
		Imp::askDaemonToRereadConfig();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('i') != NULL || opt.getValue("ignore") != NULL)
	{
		std::string dir = std::string(opt.getValue("ignore"));
		DB::addDir(DB::DirAction::ADD, DB::DirType::IGNORE, dir);
		Imp::askDaemonToRereadConfig();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('p') != NULL || opt.getValue("pay_attention") != NULL)
	{
		std::string dir = std::string(opt.getValue("pay_attention"));
		DB::addDir(DB::DirAction::REMOVE, DB::DirType::IGNORE, dir);
		Imp::askDaemonToRereadConfig();
		exit(EXIT_SUCCESS);
	}

	if (opt.getValue('l') != NULL || opt.getValue("lists") != NULL)
	{
		DB::get_directories(paths_to_monitor, paths_to_ignore);
		std::println("\nMONITOR:");
		for (auto& path: paths_to_monitor)
		{
			std::println("{}", path);
		}

		std::println("\nIGNORE:");
		for (auto& path: paths_to_ignore)
		{
			std::println("{}", path);
		}

	}

	return 0;
}
