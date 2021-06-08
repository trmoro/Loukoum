#include "Vulkan.h"

namespace Loukoum
{
	/// <summary>
	/// Constructor : init VkInstance
	/// </summary>
	Vulkan::Vulkan()
	{
		createInstance();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	/// <summary>
	/// Destructor : clean up
	/// </summary>
	Vulkan::~Vulkan()
	{
		vkDestroyDevice(m_logicalDevice, nullptr);
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
			throw std::runtime_error("failed to create instance!");
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
		createInfo.enabledExtensionCount = 0;

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