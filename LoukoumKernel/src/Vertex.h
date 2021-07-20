#pragma once

#include "glm/glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <array>
#include <vector>

namespace Loukoum
{
	class Vertex
	{
	public:

		//Constructor and destructor
		Vertex();
		Vertex(glm::vec3 pos, glm::vec4 color);
		~Vertex();

		//Set physical and logical device
		static void setDevices(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

		//Vertices
		static void AddVertex(glm::vec3 pos, glm::vec4 color);
		static std::vector<Vertex> Vertices;

		//Vertex buffer
		static void createVertexBuffer();
		static VkBuffer getVertexBuffer();
		static VkDeviceMemory getVertexBufferMemory();

		//Vulkan descriptions
		static size_t getVertexSize();
		static VkVertexInputBindingDescription getBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

		//Attributes
		glm::vec3 pos;
		glm::vec4 color;

	private:
		static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		static VkPhysicalDevice PhysicalDevice;
		static VkDevice LogicalDevice;
		static VkBuffer VertexBuffer;
		static VkDeviceMemory VertexBufferMemory;
	};
}
