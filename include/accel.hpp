#pragma once
#include "buffer.hpp"

struct AccelStruct{
    vk::UniqueAccelerationStructureKHR accel;
    Buffer buffer;

    void init(
        vk::PhysicalDevice physicalDevice, vk::Device& device,
        vk::CommandPool& commandPool, vk::Queue queue,
        vk::AccelerationStructureTypeKHR type,
        vk::AccelerationStructureGeometryKHR geometry,
        uint32_t primitiveCount
    );
};