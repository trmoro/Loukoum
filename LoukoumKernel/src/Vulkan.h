#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>

//#include "Shader.h"

#include "Utils.h"

namespace Loukoum
{
	#ifdef NDEBUG
		constexpr bool enableValidationLayers = false;
	#else
		constexpr bool enableValidationLayers = true;
	#endif

	constexpr int SHADER_VERTEX = 0;
	constexpr int SHADER_FRAGMENT = 1;

	/// <summary>
	/// Queue Family indices
	/// </summary>
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() {
			return graphicsFamily.has_value();
		}
	};

	/// <summary>
	/// Swapchain Support Details
	/// </summary>
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/// <summary>
	/// GPU Utility Class
	/// </summary>
	class GPU
	{
	public:
		GPU(std::string name,int score,VkPhysicalDevice device);
		std::string getName() const;
		int getScore() const;
		VkPhysicalDevice getDevice() const;
	private:
		std::string name;
		int score;
		VkPhysicalDevice device;
	};

	/// <summary>
	/// Vulkan Manager Class
	/// </summary>
	class Vulkan
	{
	public:
		Vulkan(GLFWwindow* window);
		~Vulkan();

		//GPU
		void printGPUsData();

		//Create Shader
		//Shader* createShader(std::string vertexFilename, std::string fragmentFilename);

		//Getters / Setters
		VkInstance getInstance() const;
	private:

		//Validation Layers
		bool checkValidationLayerSupport();
		
		//Instance creation
		void createInstance();

		//GPU
		void pickPhysicalDevice();
		void rateGPUs(std::vector<VkPhysicalDevice> devices);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void createLogicalDevice();

		//Instance and surface
		VkInstance m_instance;
		VkSurfaceKHR m_surface;
		GLFWwindow* m_window;

		//GPU
		std::vector<GPU*> m_allGPU;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; 
		VkDevice m_logicalDevice;
		VkQueue m_graphicsQueue;
		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		//Swapchain
		void createSwapchain();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);

		//Swapchain variables
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent;

		//Image view
		void createImageViews();
		std::vector<VkImageView> m_swapChainImageViews;

		//Shaders
		VkPipelineShaderStageCreateInfo createShaderStage(std::string filename, int type);
		//std::vector<Shader*> m_shaders;
		std::vector<VkShaderModule> m_shaderModules;

		//Render pass
		VkRenderPass m_renderPass;
		void createRenderPass();

		//Pipeline
		void createPipeline();
		VkPipeline m_graphicsPipeline;
		VkPipelineLayout m_pipelineLayout;

		//Validation Layers
		const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	};

}
