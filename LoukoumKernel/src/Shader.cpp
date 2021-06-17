#include "Shader.h"
#include "Utils.h"

namespace Loukoum
{
	/// <summary>
	/// Shader constructor
	/// </summary>
	/// <param name="device"></param>
	/// <param name="vertexFile"></param>
	/// <param name="fragmentFile"></param>
	Shader::Shader(VkDevice device, std::string vertexFile, std::string fragmentFile)
	{
		m_device = device;
		m_vertexFile = vertexFile;
		m_fragmentFile = fragmentFile;

		build();
	}

	/// <summary>
	/// Destructor
	/// </summary>
	Shader::~Shader()
	{
		vkDestroyShaderModule(m_device, m_vertexModule, nullptr);
		vkDestroyShaderModule(m_device, m_fragmentModule, nullptr);
	}

	/// <summary>
	/// Build shader
	/// </summary>
	void Shader::build()
	{
		m_vertexModule = createShaderModule(m_device,Utils::readFileBytecode(m_vertexFile));
		m_fragmentModule = createShaderModule(m_device, Utils::readFileBytecode(m_vertexFile));

		m_vertexShader = createVertexShader(m_vertexModule);
		m_fragmentShader = createFragmentShader(m_fragmentModule);
	}

	/// <summary>
	/// Get Vertex Shader Stage
	/// </summary>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo Shader::getVertexShaderStage()
	{
		return m_vertexShader;
	}

	/// <summary>
	/// Get Fragment Shader Stage
	/// </summary>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo Shader::getFragmentShaderStage()
	{
		return m_fragmentShader;
	}

	/// <summary>
	/// Get Shader Stages
	/// </summary>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo* Shader::getShaderStages()
	{
		return new VkPipelineShaderStageCreateInfo[2]{m_vertexShader,m_fragmentShader};
	}

	/// <summary>
	/// Create Shader Module from byte code
	/// </summary>
	/// <param name="device"></param>
	/// <param name="code"></param>
	/// <returns></returns>
	VkShaderModule Shader::createShaderModule(VkDevice device, const std::vector<char>& code)
	{
		//Create Info
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		//Create
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		//Return
		return shaderModule;
	}

	/// <summary>
	/// Create Vertex Shader from shader module
	/// </summary>
	/// <param name="module"></param>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo Shader::createVertexShader(VkShaderModule module)
	{
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageInfo.module = module;
		shaderStageInfo.pName = "main";
		return shaderStageInfo;
	}

	/// <summary>
	/// Create Vertex Shader from shader module
	/// </summary>
	/// <param name="module"></param>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo Shader::createFragmentShader(VkShaderModule module)
	{
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageInfo.module = module;
		shaderStageInfo.pName = "main";
		return shaderStageInfo;
	}
}