#include "../include/globals.hpp"
#include "../include/buffer.hpp"

void createUniformBuffer(){
    
    vk::BufferUsageFlags bufferUsage{vk::BufferUsageFlagBits::eUniformBuffer};
    vk::MemoryPropertyFlags memoryProperty{
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent};

    sceneBuffer.init(
        physicalDevice, *device, sizeof(SceneUBO),
        bufferUsage, memoryProperty, &scene);

    uniformData = device->mapMemory(sceneBuffer.memory.get(), 0, VK_WHOLE_SIZE);
}