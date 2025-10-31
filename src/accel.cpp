#include "../include/accel.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

void AccelStruct::init(
    vk::PhysicalDevice physicalDevice, vk::Device& device,
    vk::CommandPool& commandPool, vk::Queue queue,
    vk::AccelerationStructureTypeKHR type,
    vk::AccelerationStructureGeometryKHR geometry,
    uint32_t primitiveCount)
{
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.setType(type);
    buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
    buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    buildInfo.setGeometries(geometry);

    vk::AccelerationStructureBuildSizesInfoKHR buildSizes =
        device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfo, primitiveCount);
    
    buffer.init(
        physicalDevice, device, buildSizes.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.setBuffer(*buffer.buffer);
    createInfo.setSize(buildSizes.accelerationStructureSize);
    createInfo.setType(type);
    accel = device.createAccelerationStructureKHRUnique(createInfo);

    Buffer scratchBuffer;
    scratchBuffer.init(physicalDevice, device, buildSizes.buildScratchSize,
                    vk::BufferUsageFlagBits::eStorageBuffer |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);

    buildInfo.setDstAccelerationStructure(*accel);
    buildInfo.setScratchData(scratchBuffer.address);

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.setPrimitiveCount(primitiveCount);
    buildRangeInfo.setPrimitiveOffset(0);
    buildRangeInfo.setFirstVertex(0);
    buildRangeInfo.setTransformOffset(0);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    allocInfo.setCommandPool(commandPool);
    allocInfo.setCommandBufferCount(1);

    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(allocInfo);
    vk::CommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdBufs[0]->begin(cmdBeginInfo);
    cmdBufs[0]->buildAccelerationStructuresKHR(buildInfo, &buildRangeInfo);
    cmdBufs[0]->end();
    vk::CommandBuffer submitCmdBufs[1] = {cmdBufs[0].get()};
    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(submitCmdBufs);

    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.setAccelerationStructure(*accel);
    buffer.address = device.getAccelerationStructureAddressKHR(addressInfo);
}