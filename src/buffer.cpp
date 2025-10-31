#include "../include/buffer.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>


void Buffer::init(
    vk::PhysicalDevice physicalDevice,
    vk::Device& device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags memoryProperty,
    const void* data)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.setSize(size);
    createInfo.setUsage(usage);
    buffer = device.createBufferUnique(createInfo);

    vk::MemoryAllocateFlagsInfo allocateFlags{};
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        allocateFlags.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
    }

    vk::MemoryRequirements memoryReq = device.getBufferMemoryRequirements(*buffer);

    vk::MemoryAllocateInfo allocateInfo{};

    uint32_t memoryTypeIndex = 0;
    for(uint32_t i = 0; i < physicalDevice.getMemoryProperties().memoryTypeCount; i++){
        if(memoryReq.memoryTypeBits & (1 << i)
                && physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & memoryProperty){
            memoryTypeIndex = i;
            allocateInfo.setMemoryTypeIndex(memoryTypeIndex);
            break;
        }
    }
    allocateInfo.setAllocationSize(memoryReq.size);
    allocateInfo.setMemoryTypeIndex(memoryTypeIndex);
    allocateInfo.setPNext(&allocateFlags);
    memory = device.allocateMemoryUnique(allocateInfo);

    device.bindBufferMemory(*buffer, *memory, 0);
    if (data) {
        void* mappedPtr = device.mapMemory(*memory, 0, size);
        memcpy(mappedPtr, data, size);
        device.unmapMemory(*memory);
    }
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        vk::BufferDeviceAddressInfoKHR addressInfo{};
        addressInfo.setBuffer(*buffer);
        address = device.getBufferAddressKHR(&addressInfo);
    }
}