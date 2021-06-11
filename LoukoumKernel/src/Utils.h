#pragma once

#include <vector>
#include <fstream>

namespace Loukoum
{
	class Utils
	{
	public:
		static std::vector<char> readFileBytecode(const std::string& filename);
	};
}