#include "Vulkan.h"

namespace Loukoum
{
	/// <summary>
	/// Constructor : init VkInstance
	/// </summary>
	Vulkan::Vulkan(GLFWwindow* window)
	{
		m_window = window;
		//m_shaders = std::vector<Shader*>();
		m_shaderModules = std::vector<VkShaderModule>();
		m_vertices = std::vector<Vertex>();

		createInstance();
		pickPhysicalDevice();
		createLogicalDevice();
		createCommandPool();
		recreateSwapChain();
		createSyncObjects();
	}

	/// <summary>
	/// Destructor : clean up
	/// </summary>
	Vulkan::~Vulkan()
	{
		vkDeviceWaitIdle(m_logicalDevice);
		cleanUpSwapChain();

		vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
		vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);

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
	/// Draw Frame
	/// </summary>
	void Vulkan::drawFrame()
	{
		//Wait all fences
		vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

		//Get image index from swapchain
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

		//Image not compatible with window
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to present image to the swapchain");
		}

		//If frame still in use, wait
		if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		//The new current frame is now in use
		m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

		//Prepare a command to get image
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

		//Link render finished semaphore
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		//Submit command
		vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);
		if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to send a Command Buffer");
		}

		//Presentation Image info
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		//Link Swapchains to Prensentation Info
		VkSwapchainKHR swapChains[] = { m_swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		//Show image
		result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
			m_framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present an image");
		}

		//Next frame
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	/// <summary>
	/// Create Vertex Buffer
	/// </summary>
	void Vulkan::createVertexBuffer()
	{
		//Buffer info
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(Vertex) * m_vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//Create Buffer
		if (vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("(Vertex Class) Failed to create vertex buffer!");
		}

		//Memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_logicalDevice, m_vertexBuffer, &memRequirements);

		///Memory allocation info
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		//Allocation
		if (vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("(Vertex Class) Failed to allocate vertex buffer memory!");
		}

		//Bind Buffer with memory
		vkBindBufferMemory(m_logicalDevice, m_vertexBuffer, m_vertexBufferMemory, 0);

		void* data;
		vkMapMemory(m_logicalDevice, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, m_vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(m_logicalDevice, m_vertexBufferMemory);
	}

	/// <summary>
	/// Add vertex
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="color"></param>
	void Vulkan::addVertex(glm::vec3 pos, glm::vec4 color)
	{
		m_vertices.push_back({ pos, color });
	}

	/// <summary>
	/// Recreate Swapchain
	/// </summary>
	void Vulkan::recreateSwapChain()
	{
		//Get size from GLFW
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(m_window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_logicalDevice);

		createSwapchain();
		createImageViews();
		createRenderPass();
		createPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	/// <summary>
	/// Create Shader
	/// </summary>
	/// <param name="vertexFilename">>Vertex filename</param>
	/// <param name="fragmentFilename">Framgnet filename</param>
	/// <returns></returns> 
	/*
	Shader* Vulkan::createShader(std::string vertexFilename, std::string fragmentFilename)
	{
		Shader* shader = new Shader(m_logicalDevice, vertexFilename, fragmentFilename);
		m_shaders.push_back(shader);
		return shader;
	}
	*/

	/// <summary>
	/// Get Vulkan Instance
	/// </summary>
	/// <returns>VkInstance object</returns>
	VkInstance Vulkan::getInstance() const
	{
		return m_instance;
	}

	/// <summary>
	/// Set Frame Resized
	/// </summary>
	/// <param name="b">boolean</param>
	void Vulkan::setFrameResized(bool b)
	{
		m_framebufferResized = b;
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

			//Present family
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			//All queue family
			if (indices.isComplete()) {
				break;
			}
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
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//Features to use
		VkPhysicalDeviceFeatures deviceFeatures{};

		//Create device info
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		//Link queue info
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

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

		//Get Graphics and present Queue
		vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
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
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

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
	VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(m_window, &width, &height);
			VkExtent2D actualExtent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	/// <summary>
	/// Clean Up Swapchain
	/// </summary>
	void Vulkan::cleanUpSwapChain()
	{
		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);
		}

		vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

		vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_logicalDevice, imageView, nullptr);
		}

		for (VkShaderModule shader : m_shaderModules)
			vkDestroyShaderModule(m_logicalDevice, shader, nullptr);

		vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);

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

	/// <summary>
	/// Create Shader Stage
	/// </summary>
	/// <param name="filename"></param>
	/// <param name="type"></param>
	/// <returns></returns>
	VkPipelineShaderStageCreateInfo Vulkan::createShaderStage(std::string filename, int type)
	{
		//Read file
		const std::vector<char>& code = Utils::readFileBytecode(filename);

		//Create Info
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		//Create Module
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}
		m_shaderModules.push_back(shaderModule);

		//Create stage
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";

		if(type == SHADER_VERTEX)
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		else if(type == SHADER_FRAGMENT)
			shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		return shaderStageInfo;

	}

	/// <summary>
	/// Create Render Pass
	/// </summary>
	void Vulkan::createRenderPass()
	{
		//Color Attachment
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//Stencil / Depth
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//How input and output image are
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//Attachment reference
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//Subpass
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		//Create Render Pass
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		//Subpass dependencies
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		//Create Render Pass
		if (vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Render Pass");
		}
	}

	/// <summary>
	/// Create Pipeline
	/// </summary>
	void Vulkan::createPipeline()
	{
		//Test shader
		VkPipelineShaderStageCreateInfo vert = createShaderStage("C:/Users/trist/Documents/VS_Project/Loukoum/x64/Debug/shaders/test.vert.spv", SHADER_VERTEX);
		VkPipelineShaderStageCreateInfo frag = createShaderStage("C:/Users/trist/Documents/VS_Project/Loukoum/x64/Debug/shaders/test.frag.spv", SHADER_FRAGMENT);
		VkPipelineShaderStageCreateInfo shaderStages[] = { vert, frag };

		//Vertex input
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		//Input assembly : how vertices are linked
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//Viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainExtent.width;
		viewport.height = (float)m_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		//Scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChainExtent;

		//Viewport state info (Viewport + scissor)
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		//Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		//Color blend Attachment
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		//Color blending
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		//Dynamic states
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		//Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		//Create Pipeline Layout
		if (vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Pipeline layout");
		}

		//Create Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;

		//Link pipeline layout and render pas
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;

		//No second pipeline
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		//Create pipeline
		if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphical pipeline");
		}
	}

	/// <summary>
	/// Create Framebuffers
	/// </summary>
	void Vulkan::createFramebuffers()
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		//Create framebuffer for each image view
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {
				m_swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer");
			}
		}
	}

	/// <summary>
	/// Create Command Pool
	/// </summary>
	void Vulkan::createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0;
		if (vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Pool");
		}
	}

	/// <summary>
	/// Create Command Buffers
	/// </summary>
	void Vulkan::createCommandBuffers()
	{
		m_commandBuffers.resize(m_swapChainFramebuffers.size());

		//Buffer allocation
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

		if (vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffer!");
		}

		//Foreach command buffer
		for (size_t i = 0; i < m_commandBuffers.size(); i++) {

			//Start recording of command buffer
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("Failed to start command buffer recording!");
			}

			//Start a render pass info
			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_renderPass;
			renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_swapChainExtent;

			//Render pass clear color
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			//Begin Render pass
			vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			//Bind Pipeline and draw
			vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
			VkBuffer vertexBuffers[] = {m_vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdDraw(m_commandBuffers[i], static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);

			//Finish render
			vkCmdEndRenderPass(m_commandBuffers[i]);
			if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to end command buffer recording");
			}
		}

	}

	/// <summary>
	/// Create Semaphores
	/// </summary>
	void Vulkan::createSyncObjects()
	{
		//Resize semaphores and fences
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

		//Info
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		//Creates
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
		{
			if (vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			{

				throw std::runtime_error("Failed to create sync objects (semaphores and fences)");
			}
		}
	}

	/// <summary>
	/// Find suitable memory type for memory allocation
	/// </summary>
	/// <param name="typeFilter"></param>
	/// <param name="properties"></param>
	/// <returns></returns>
	uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");

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