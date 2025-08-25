#ifndef XATTR_H
#define XATTR_H

#include <string>
#include <vector>
#include <optional>
#include <sys/types.h>
#include <sys/xattr.h>
#include <cstring>
#include <fstream>
#include <print>

class XAttr
{
public:

	static bool isWorking()
	{
		std::string attr_name {"whatmade"};
		std::string attr_value {"works"};
		std::string path {"/tmp/whatmade.tst"};
		bool result;

		std::ofstream test_file_stream(path);
		test_file_stream.close();

		if (set(path, attr_name, attr_value))
		{
			std::optional<std::string> get_result = get(path, attr_name);
			if (get_result && get_result.value() == attr_value)
			{
				result = true;
			}else
			{
				result = false;
			}
		}else
		{
			result = false;
		}

		remove(path.c_str());

		return result;
	}

	static bool set(const std::string& path,
					const std::string& name,
					const std::string& value,
					int flags = 0)
	{
		std::string correct_attr_name = std::string("user.") + name;
		if (setxattr(path.c_str(), correct_attr_name.c_str(),
					   value.data(), value.size(), flags) == -1)
		{
			return false;
		}

		return true;
	}

	static std::optional<std::string> get(const std::string& path,
						   const std::string& name)
	{
		std::string correct_attr_name = std::string("user.") + name;
		// First, query size
		ssize_t size = getxattr(path.c_str(), correct_attr_name.c_str(), nullptr, 0);
		if (size == -1)
		{
			return std::nullopt;
		}

		std::vector<char> buf(size);
		ssize_t ret = getxattr(path.c_str(), correct_attr_name.c_str(), buf.data(), buf.size());
		if (ret == -1)
		{
			return std::nullopt;
		}

		std::string result (buf.begin(), buf.begin() + ret);

		std::println("{}", result);

		return result;
	}
};

#endif // XATTR_H
