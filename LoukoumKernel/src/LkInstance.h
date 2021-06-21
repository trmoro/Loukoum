#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <fstream>
#include <iostream>

#include "Vulkan.h"

namespace Loukoum
{
	class LkInstance
	{
	public:
		LkInstance();
		~LkInstance();

		//Run
		void run();

		//GLFW callbacks
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	private:

		//Vulkan Manager
		Vulkan* m_vulkan;

		//Loukoum methods
		void initWindow();
		void initVulkan();
		void mainLoop();
		void cleanUp();

		//GLFW Window
		GLFWwindow* m_window;

		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;
	};
}