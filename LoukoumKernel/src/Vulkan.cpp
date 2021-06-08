#include "Vulkan.h"

namespace Loukoum
{
	/// <summary>
	/// Constructor : init VkInstance
	/// </summary>
	Vulkan::Vulkan()
	{
		createInstance();
	}

	/// <summary>
	/// Destructor : clean up
	/// </summary>
	Vulkan::~Vulkan()
	{
		vkDestroyInstance(m_instance, nullptr);
	}

	/// <summary>
	/// Get Vulkan Instance
	/// </summary>
	/// <returns>VkInstance object</returns>
	VkInstance Vulkan::getInstance() const
	{
		return m_instance;
	}

	/// <summary>
	/// Check if the chosen validation layer are supported
	/// </summary>
	/// <returns>a boolean</returns>
	bool Vulkan::checkValidationLayerSupport()
	{
		//Get validation layer count
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		//Get validation layers
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		//Check if all chosen validation layer are supported
		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	/// <summary>
	/// Create Vulkan Instance
	/// </summary>
	void Vulkan::createInstance()
	{
		//Validation layer supported ?
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layer activated but not supported");
		}

		//App info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Loukoum App";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//Create info
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//Add validation layer if activated
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		//Get GLFW extension
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		//Create Vulkan Instance
		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
}