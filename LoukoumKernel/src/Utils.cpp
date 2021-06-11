#include "Utils.h"

/// <summary>
/// Read FIle as Bytecode
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
std::vector<char> Loukoum::Utils::readFileBytecode(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file");
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}
