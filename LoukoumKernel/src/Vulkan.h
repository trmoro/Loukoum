#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cstring>

namespace Loukoum
{
	#ifdef NDEBUG
		constexpr bool enableValidationLayers = false;
	#else
		constexpr bool enableValidationLayers = true;
	#endif

	class Vulkan
	{
	public:
		Vulkan();
		~Vulkan();
		VkInstance getInstance() const;
	private:

		bool checkValidationLayerSupport();
		void createInstance();

		VkInstance m_instance;

		const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	};

}
