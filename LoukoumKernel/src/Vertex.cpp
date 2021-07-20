#include "Vertex.h"

namespace Loukoum
{
	/// <summary>
	/// Vertex constructor
	/// </summary>
	Vertex::Vertex()
	{
		pos = glm::vec3();
		color = glm::vec4();
	}
	
	/// <summary>
	/// Vertex constructor with fields
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="color"></param>
	Vertex::Vertex(glm::vec3 pos, glm::vec4 color)
	{
		this->pos = pos;
		this->color = color;
	}

	/// <summary>
	/// Empty vertex destructor
	/// </summary>
	Vertex::~Vertex()
	{
	}

	/// <summary>
	/// Set Physical and Logical Device
	/// </summary>
	/// <param name="physicalDevice"></param>
	/// <param name="logicalDevice"></param>
	void Vertex::setDevices(VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		PhysicalDevice = physicalDevice;
		LogicalDevice = logicalDevice;
	}

	/// <summary>
	/// Add Vertex
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="color"></param>
	void Vertex::AddVertex(glm::vec3 pos, glm::vec4 color)
	{
		Vertices.push_back(Vertex(pos, color));
	}

	/// <summary>
	/// Create Vertex Buffer
	/// </summary>
	void Vertex::createVertexBuffer()
	{
		//Buffer info
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = getVertexSize() * Vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//Create Buffer
		if (vkCreateBuffer(LogicalDevice, &bufferInfo, nullptr, &VertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("(Vertex Class) Failed to create vertex buffer!");
		}

		//Memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(LogicalDevice, VertexBuffer, &memRequirements);

		///Memory allocation info
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		//Allocation
		if (vkAllocateMemory(LogicalDevice, &allocInfo, nullptr, &VertexBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("(Vertex Class) Failed to allocate vertex buffer memory!");
		}

		//Bind Buffer with memory
		vkBindBufferMemory(LogicalDevice, VertexBuffer, VertexBufferMemory, 0);

		void* data;
		vkMapMemory(LogicalDevice, VertexBufferMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, Vertices.data(), (size_t)bufferInfo.size);
		vkUnmapMemory(LogicalDevice, VertexBufferMemory);
	}

	/// <summary>
	/// Return Vertex Buffer
	/// </summary>
	/// <returns></returns>
	VkBuffer Vertex::getVertexBuffer()
	{
		return VertexBuffer;
	}

	/// <summary>
	/// Get Vertex Buffer Memory
	/// </summary>
	/// <returns></returns>
	VkDeviceMemory Vertex::getVertexBufferMemory()
	{
		return VertexBufferMemory;
	}

	/// <summary>
	/// Get Vertex Size
	/// </summary>
	/// <returns></returns>
	size_t Vertex::getVertexSize()
	{
		return sizeof(glm::vec3) + sizeof(glm::vec4);
	}

	/// <summary>
	/// Get Binding Description
	/// </summary>
	/// <returns></returns>
	VkVertexInputBindingDescription Vertex::getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = getVertexSize();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	/// <summary>
	/// Get Attribute Descriptions
	/// </summary>
	/// <returns></returns>
	std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		//Color
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	/// <summary>
	/// Find suitable memory type for memory allocation
	/// </summary>
	/// <param name="typeFilter"></param>
	/// <param name="properties"></param>
	/// <returns></returns>
	uint32_t Vertex::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("(Vertex Class) Failed to find suitable memory type!");
	}
}