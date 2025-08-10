#pragma once

#include <sqlite3.h>
#include <print>
#include <vector>
#include <set>
#include "os_specific.h"

class DB
{
public:
	enum class DirType {MONITOR, IGNORE};
	enum class DirAction {ADD, REMOVE};
	static void connect();
	static void silent_query(const std::string& command);
	static sqlite3_stmt* query(std::string& command);
	static void add_filename_process_pair(const std::string& filename, const std::string& processname);
	static void remove_filename(const std::string& filename);
	static std::string get_process_by_filename(const std::string& filename);
	static std::vector<std::string>	get_all_filenames();
	static void remove_these_filenames(const std::vector<std::string>& filenames);
	static void get_directories(std::set<std::string>& paths_to_control, std::set<std::string>& paths_to_ignore);
	static void addDir(DirAction, DirType, const std::string& dir);

	inline static sqlite3* db = nullptr;
	inline static const char* DB_FILE = "/var/lib/whomade/IKnowWhoCreatedThisFile.db";
	inline static const char* DB_DIR = "/var/lib/whomade";
};


sqlite3_stmt* DB::query(std::string& command)
{
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db, command.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
	{
			std::println("Failed to prepare statement: {}", sqlite3_errmsg(db));
			sqlite3_close(db);
			return nullptr;
	}

	return stmt;
}

void DB::silent_query(const std::string& command)
{
	char *zErrMsg = nullptr;
	int rc = sqlite3_exec(DB::db, command.c_str(), nullptr, 0, &zErrMsg);
	if( rc != SQLITE_OK )
	{
		if (rc == SQLITE_CONSTRAINT)
		{
			// that's ok. Just trying to write down some filename again.
		}else
		{
			std::println("db_silent_exec() error: rc={0}\n descrition={1}\n query={2}", rc, sqlite3_errmsg(DB::db), command);
		}
		sqlite3_free(zErrMsg);
	}
}


void DB::connect()
{
	int rc = sqlite3_open_v2(DB_FILE, &DB::db, SQLITE_OPEN_READWRITE, nullptr);

	if( rc == SQLITE_OK)
	{
		std::println("Database is opened successfully.");
	}
	else
	{
		std::println("No DB. Creating new.");
		mkdir(DB_DIR, 0755);
		rc = sqlite3_open(DB_FILE, &DB::db);
		if (rc != SQLITE_OK)
		{
			std::println("Can't create new DB. Error: {}, {}", rc, sqlite3_errmsg(DB::db));
			exit(EXIT_FAILURE);
		}

		std::vector<std::string> table_creating_commands;
		table_creating_commands.assign(3, "");
		table_creating_commands[0] = "CREATE table if not exists Files (filename text NOT NULL, processname text, PRIMARY KEY (filename))";
		table_creating_commands[1] = "CREATE table if not exists DirsToMonitor (dirname text NOT NULL, PRIMARY KEY (dirname));";
		table_creating_commands[2] = "CREATE table if not exists DirsToIgnore  (dirname text NOT NULL, PRIMARY KEY (dirname));";

		for (const std::string& command: table_creating_commands)
		{
			silent_query(command);
		}

		addDir(DirAction::ADD, DirType::MONITOR, "/home/astahl/.config");
		addDir(DirAction::ADD, DirType::MONITOR, "/home/astahl/.cache");
		addDir(DirAction::ADD, DirType::IGNORE,  "/home/astahl/.cache/mozilla");
	}
}

void DB::addDir(DirAction action, DirType dir_type, const std::string& dir)
{
	std::string table_name;
	switch (dir_type)
	{
		case DirType::MONITOR: table_name = "DirsToMonitor"; break;
		case DirType::IGNORE:  table_name = "DirsToIgnore"; break;
	}

	std::string query;

	switch (action)
	{
		case DirAction::ADD:
			query = std::format(
						R"SQL(

						INSERT INTO {0:}(dirname)
						SELECT '{1:}'
						WHERE NOT EXISTS
						(SELECT 1
						 FROM {0:}
						 WHERE dirname = '{1:}')

						)SQL", table_name, dir);
			break;

		case DirAction::REMOVE:
			query = std::format("DELETE FROM {} WHERE dirname='{}';", table_name, dir);
	}

	silent_query(query);

}

void DB::add_filename_process_pair(const std::string& filename, const std::string& processname)
{
	std::string query = std::format("INSERT into Files (filename, processname) values ('{}', '{}');", filename, processname);
	silent_query(query);
}

void DB::get_directories(std::set<std::string>& paths_to_monitor, std::set<std::string>& paths_to_ignore)
{
	std::string query_monitor {"SELECT * FROM DirsToMonitor;"};
	sqlite3_stmt* stmt_monitor = query(query_monitor);

	paths_to_monitor.clear();
	while (sqlite3_step(stmt_monitor) == SQLITE_ROW)
	{
		const unsigned char* path = sqlite3_column_text(stmt_monitor, 0);
		std::string path_str(reinterpret_cast<const char*>(path));
		paths_to_monitor.insert(path_str);

	}
	sqlite3_finalize(stmt_monitor);

	std::string query_ignore {"SELECT * FROM DirsToIgnore;"};
	sqlite3_stmt* stmt_ignore = query(query_ignore);

	paths_to_ignore.clear();
	while (sqlite3_step(stmt_ignore) == SQLITE_ROW)
	{
		const unsigned char* path = sqlite3_column_text(stmt_ignore, 0);
		std::string path_str(reinterpret_cast<const char*>(path));
		paths_to_ignore.insert(path_str);

	}
	sqlite3_finalize(stmt_ignore);
}

std::string DB::get_process_by_filename(const std::string& filename)
{
	std::string result{};
	std::string query_str = std::format("SELECT (processname) FROM Files WHERE filename='{}'", filename);
	sqlite3_stmt* result_stmt = query(query_str);
	if (sqlite3_step(result_stmt) == SQLITE_ROW)
	{
		const unsigned char* processname = sqlite3_column_text(result_stmt, 0);
		result = reinterpret_cast<const char*>(processname);
	}
	sqlite3_finalize(result_stmt);
	return result;
}

std::vector<std::string> DB::get_all_filenames()
{
	std::vector<std::string> result;
	std::string query_str = std::format("SELECT (filename) FROM Files;");
	sqlite3_stmt* result_stmt = query(query_str);
	while (sqlite3_step(result_stmt) == SQLITE_ROW)
	{
		const unsigned char* filename = sqlite3_column_text(result_stmt, 0);
		std::string filename_str(reinterpret_cast<const char*>(filename));
		result.push_back(filename_str);

	}
	sqlite3_finalize(result_stmt);

	return result;
}

void DB::remove_these_filenames(const std::vector<std::string>& filenames)
{
	std::string query_str = std::format("DELETE FROM Files WHERE filename IN ( ");
	for (const auto& filename: filenames)
	{
		query_str += std::format("'{}',", filename);
	}
	query_str.pop_back(); // last comma
	query_str += ");";

	silent_query(query_str);
}
