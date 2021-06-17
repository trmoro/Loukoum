#pragma once

#include <vector>
#include <fstream>
#include <iostream>

namespace Loukoum
{
	class Utils
	{
	public:
		static std::vector<char> readFileBytecode(const std::string& filename);
	};
}