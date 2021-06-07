#include "LkInstance.h"

namespace Loukoum
{

	/// <summary>
	/// Loukoum Instance Constructor
	/// </summary>
	Loukoum::LkInstance::LkInstance()
	{
	}

	/// <summary>
	/// Loukoum Instance Destructor
	/// </summary>
	Loukoum::LkInstance::~LkInstance()
	{
	}

	/// <summary>
	/// Debug test
	/// </summary>
	void Loukoum::LkInstance::debug()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::cout << extensionCount << " extensions supported\n";

		glm::mat4 matrix;
		glm::vec4 vec;
		auto test = matrix * vec;

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}

		glfwDestroyWindow(window);

		glfwTerminate();
	}

}