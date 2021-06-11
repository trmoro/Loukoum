#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <string>

namespace Loukoum
{
	class Shader
	{
	public:
		Shader(VkDevice device, std::string vertexFile, std::string fragmentFile);
		~Shader();

		//Build
		void build();

		//Shader static functions
		static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
		static VkPipelineShaderStageCreateInfo createVertexShader(VkShaderModule module);
		static VkPipelineShaderStageCreateInfo createFragmentShader(VkShaderModule module);

	private:

		std::string m_vertexFile;
		std::string m_fragmentFile;

		VkDevice m_device;

		VkShaderModule m_vertexModule;
		VkShaderModule m_fragmentModule;

		VkPipelineShaderStageCreateInfo m_vertexShader;
		VkPipelineShaderStageCreateInfo m_fragmentShader;
	};
}