#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <array>

#include <glm/glm.hpp>

//#include "Shader.h"

#include "Utils.h"

namespace Loukoum
{
	#ifdef NDEBUG
		constexpr bool enableValidationLayers = false;
	#else
		constexpr bool enableValidationLayers = true;
	#endif

	const int MAX_FRAMES_IN_FLIGHT = 2;

	constexpr int SHADER_VERTEX = 0;
	constexpr int SHADER_FRAGMENT = 1;

	/// <summary>
	/// Queue Family indices
	/// </summary>
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
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
	/// Vertex structure
	/// </summary>
	struct Vertex {
		glm::vec3 pos;
		glm::vec4 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;

		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			//Position
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			//Color
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
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

		//Draw Frame
		void drawFrame();

		//Vertex methods
		void createVertexBuffer();
		void addVertex(glm::vec3 pos, glm::vec4 color);

		//Recreate Swapchain
		void recreateSwapChain();

		//Create Shader
		//Shader* createShader(std::string vertexFilename, std::string fragmentFilename);

		//Getters
		VkInstance getInstance() const;

		//Setters
		void setFrameResized(bool b);

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
		VkQueue m_presentQueue;
		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		//Swapchain
		void createSwapchain();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		//Swapchain recreation
		void cleanUpSwapChain();

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

		//Framebuffers
		void createFramebuffers();
		std::vector<VkFramebuffer> m_swapChainFramebuffers;

		//Command Pool and buffers
		void createCommandPool();
		void createCommandBuffers();
		VkCommandPool m_commandPool;
		std::vector<VkCommandBuffer> m_commandBuffers;

		//Semaphores and Fences : to render
		void createSyncObjects();
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;
		size_t m_currentFrame = 0;
		bool m_framebufferResized = false;

		//Vertex Methods and Variables
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		std::vector<Vertex> m_vertices;

		//Validation Layers
		const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	};

}
