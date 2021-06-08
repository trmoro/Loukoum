#include "LkInstance.h"

namespace Loukoum
{

	/// <summary>
	/// Loukoum Instance Constructor
	/// </summary>
	Loukoum::LkInstance::LkInstance()
	{
		m_window = nullptr;
		m_vulkan = nullptr;
	}

	/// <summary>
	/// Loukoum Instance Destructor
	/// </summary>
	Loukoum::LkInstance::~LkInstance()
	{
	}

	/// <summary>
	/// Run Loukoum Engine
	/// </summary>
	void LkInstance::run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}

	/// <summary>
	/// Init window
	/// </summary>
	void LkInstance::initWindow()
	{
		std::cout << "Loukoum : init window" << std::endl;

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Loukoum", nullptr, nullptr);

		std::cout << "Loukoum : init window ended" << std::endl;
	}

	/// <summary>
	/// Init Vulkan
	/// </summary>
	void LkInstance::initVulkan()
	{
		std::cout << "Loukoum : init vulkan" << std::endl;
		m_vulkan = new Vulkan(m_window);
		m_vulkan->printGPUsData();
		std::cout << "Loukoum : init vulkan ended" << std::endl;
	}

	/// <summary>
	/// Main Loop
	/// </summary>
	void LkInstance::mainLoop()
	{
		std::cout << "Loukoum : main loop" << std::endl;

		while (!glfwWindowShouldClose(m_window)) {
			glfwPollEvents();
		}

		std::cout << "Loukoum : main loop ended" << std::endl;
	}

	/// <summary>
	/// Clean Up
	/// </summary>
	void LkInstance::cleanUp()
	{
		std::cout << "Loukoum : clean up" << std::endl;

		delete m_vulkan;
		glfwDestroyWindow(m_window);
		glfwTerminate();

		std::cout << "Loukoum : clean up ended" << std::endl;
	}

}