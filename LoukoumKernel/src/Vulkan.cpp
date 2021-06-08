#include "Vulkan.h"

namespace Loukoum
{
	/// <summary>
	/// Constructor : init VkInstance
	/// </summary>
	Vulkan::Vulkan(GLFWwindow* window)
	{
		m_window = window;

		createInstance();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
	}

	/// <summary>
	/// Destructor : clean up
	/// </summary>
	Vulkan::~Vulkan()
	{
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
		vkDestroyDevice(m_logicalDevice, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	/// <summary>
	/// Print GPUs Data in the console
	/// </summary>
	void Vulkan::printGPUsData()
	{
		std::cout << std::endl;
		std::cout << "GPU Devices Available" << std::endl;

		for (GPU* gpu : m_allGPU)
		{
			std::cout << "--" << gpu->getName() << " | LkScore : " << gpu->getScore() << std::endl;
		}
		std::cout << std::endl;
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
			throw std::runtime_error("Failed to create instance!");
		}

		//Create Vulkan surface
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create surface!");
		}
	}

	/// <summary>
	/// Pick Physical Device
	/// </summary>
	void Vulkan::pickPhysicalDevice()
	{
		//Get GPU count
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("Vulkan not supported by GPU");
		}

		//Get Physical Device
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		//Rate GPUs
		rateGPUs(devices);

		//Get best gpu
		int bestScore = 0;
		int index = 0;
		for (int i = 0; i < m_allGPU.size(); i++)
		{
			if (m_allGPU[i]->getScore() > bestScore)
			{
				bestScore = m_allGPU[i]->getScore();
				index = i;
			}
		}
		if (bestScore == 0) {
			throw std::runtime_error("No suitable GPU");
		}
		else
			m_physicalDevice = m_allGPU[index]->getDevice();
	}

	//Rate GPUs
	void Vulkan::rateGPUs(std::vector<VkPhysicalDevice> devices)
	{
		//Create GPU vector
		m_allGPU = std::vector<GPU*>();

		//Foreach GPU
		for (VkPhysicalDevice device : devices)
		{
			//Get properties and features of the GPU
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			int score = 0;

			// Discrete GPUs have a significant performance advantage
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			//Maximum possible size of textures affects graphics quality
			score += deviceProperties.limits.maxImageDimension2D;

			//Application can't function without geometry shaders
			if (!deviceFeatures.geometryShader)
				score = 0;

			//Get family queue and check if indices are all set, else, set score to 0
			QueueFamilyIndices indices = findQueueFamilies(device);
			if (!indices.isComplete())
				score = 0;

			//Check if extensions are supported, else, set score to 0
			if (!checkDeviceExtensionSupport(device))
				score = 0;
			//Check swap chain
			else
			{
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
				bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
				if (!swapChainAdequate)
					score = 0;
			}

			//Add
			m_allGPU.push_back(new GPU(deviceProperties.deviceName, score, device));
		}
	}

	/// <summary>
	/// Find Queue Family indices with a given device
	/// </summary>
	/// <param name="device">VkPhysicalDevice object</param>
	/// <returns></returns>
	QueueFamilyIndices Vulkan::findQueueFamilies(VkPhysicalDevice device)
	{
		//Queue Family Indices
		QueueFamilyIndices indices;

		//Get family count
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		//Get all families queue properties
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		//For each queue family, get corresponding indices
		for (int i = 0; i < queueFamilies.size(); i++)
		{
			//Graphics family
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;
		}

		return indices;
	}

	/// <summary>
	/// Check if GPU can support the extensions chosen in deviceExtensions constant list
	/// </summary>
	/// <param name="device"></param>
	/// <returns></returns>
	bool Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		//Get extension count supported by GPU
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		//Get extensions supported by GPU
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		//Check if all chosen extensionss are supported
		for (const char* extension : deviceExtensions) {
			bool extFound = false;

			for (const auto& ext : availableExtensions) {
				if (strcmp(extension, ext.extensionName) == 0) {
					extFound = true;
					break;
				}
			}

			if (!extFound) {
				return false;
			}
		}

		return true;
	}

	/// <summary>
	/// Create Logical Device
	/// </summary>
	void Vulkan::createLogicalDevice()
	{
		//Find queue families for the chosen physical device
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		//Create device queue info
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();

		//Queue count
		queueCreateInfo.queueCount = 1;

		//Queue priority
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		//Features to use
		VkPhysicalDeviceFeatures deviceFeatures{};

		//Create device info
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		//Link queue info
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;

		//Link features
		createInfo.pEnabledFeatures = &deviceFeatures;

		//Extension enabled
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		//Validation layers
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		//Create logical device
		if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Logical Device");
		}

		//Get Graphics Queue
		vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	}

	/// <summary>
	/// Create Swapchain
	/// </summary>
	void Vulkan::createSwapchain()
	{
		//Get best swapchain parameters
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities,800,600);

		//Swapchain image count
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		//Swapchain max image count, 0 means no limit
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;
		
		//Swapchain creation info
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		//Swapchain creation info of sharing mode of queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		//Transformation applied to image (none)
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		//Present Mode
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		//Manage old swap chain
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		//Create swap chain
		if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Swapchain");
		}

		//Get swapchain images
		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
		
		//Swapchain format and extent
		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	/// <summary>
	/// Query Swap chain support of the given GPU
	/// </summary>
	/// <param name="device">VkPhysicalDevice object</param>
	/// <returns></returns>
	SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		//Surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

		//Surface format
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
		}

		//Present Mode
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	/// <summary>
	/// Choose best swapchain surface format
	/// </summary>
	/// <param name="availableFormats">Formats available</param>
	/// <returns></returns>
	VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	/// <summary>
	/// Choose best swapchain present mode
	/// </summary>
	/// <param name="availablePresentModes">Present modes availables</param>
	/// <returns></returns>
	VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	/// <summary>
	/// Choose best swapchain extent mode
	/// </summary>
	/// <param name="capabilities">Extent mode available</param>
	/// <param name="width">Surface width</param>
	/// <param name="height">Surface height</param>
	/// <returns></returns>
	VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			VkExtent2D actualExtent = { width, height };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	/// <summary>
	/// Create Image Views
	/// </summary>
	void Vulkan::createImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImages.size(); i++) 
		{
			//Create a image view
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;

			//Component of the image
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			//Only color
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create a Image view");
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// GPU utility class constructor
	/// </summary>
	/// <param name="name">Name</param>
	/// <param name="score">Score</param>
	/// <param name="device">Device</param>
	GPU::GPU(std::string name, int score, VkPhysicalDevice device)
	{
		this->name = name;
		this->score = score;
		this->device = device;
	}
	
	/// <summary>
	/// Get GPU Name
	/// </summary>
	/// <returns></returns>
	std::string GPU::getName() const
	{
		return this->name;
	}

	/// <summary>
	/// Get GPU Score
	/// </summary>
	/// <returns></returns>
	int GPU::getScore() const
	{
		return this->score;
	}

	/// <summary>
	/// Get GPU Device
	/// </summary>
	/// <returns></returns>
	VkPhysicalDevice GPU::getDevice() const
	{
		return this->device;
	}
}