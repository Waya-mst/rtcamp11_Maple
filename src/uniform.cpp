#include "../include/globals.hpp"
#include "../include/buffer.hpp"

void createUniformBuffer(){
    vk::DeviceSize bufferSize = sizeof(scene);
    
    vk::BufferUsageFlags bufferUsage{vk::BufferUsageFlagBits::eUniformBuffer};
    vk::MemoryPropertyFlags memoryProperty{
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent};

    sceneBuffer.init(
        physicalDevice, *device, sizeof(SceneUBO),
        bufferUsage, memoryProperty, &scene);
}