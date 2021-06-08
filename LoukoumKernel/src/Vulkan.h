#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>

namespace Loukoum
{
	#ifdef NDEBUG
		constexpr bool enableValidationLayers = false;
	#else
		constexpr bool enableValidationLayers = true;
	#endif

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
		Vulkan();
		~Vulkan();

		//GPU
		void printGPUsData();

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
		void createLogicalDevice();

		//Instance
		VkInstance m_instance;

		//GPU
		std::vector<GPU*> m_allGPU;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE; 
		VkDevice m_logicalDevice;
		VkQueue m_graphicsQueue;

		const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	};

}
