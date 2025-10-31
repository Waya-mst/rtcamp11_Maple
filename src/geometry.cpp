#include "../include/globals.hpp"
#include "../include/buffer.hpp"
#include "../include/accel.hpp"
#include <iostream>

void createBLAS(){

    vk::BufferUsageFlags bufferUsage{
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
        vk::BufferUsageFlagBits::eStorageBuffer};
    vk::MemoryPropertyFlags memoryProperty{
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent};

    vertexBuffer.init(
        physicalDevice, *device, vertices.size() * sizeof(vertices[0]),
        bufferUsage, memoryProperty, vertices.data());

    indexBuffer.init(
        physicalDevice, *device, indices.size() * sizeof(uint32_t),
        bufferUsage, memoryProperty, indices.data());

    vk::BufferDeviceAddressInfoKHR vertAddress{};
    vk::BufferDeviceAddressInfoKHR indexAddress{};
    vertAddress.setBuffer(vertexBuffer.buffer.get());
    indexAddress.setBuffer(indexBuffer.buffer.get());
    
    vk::AccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    
    triangles.setVertexData(device->getBufferAddressKHR(&vertAddress));
    triangles.setVertexStride(sizeof(Vertex));
    triangles.setMaxVertex(static_cast<uint32_t>(vertices.size()));
    triangles.setIndexType(vk::IndexType::eUint32);
    triangles.setIndexData(device->getBufferAddressKHR(&indexAddress));
    
    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometry.setGeometry({triangles});
    geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    
    uint32_t primitiveCount = static_cast<uint32_t>(indices.size() / 3);
    bottomAccel.init(
        physicalDevice, *device, commandPool.get(), queue,
        vk::AccelerationStructureTypeKHR::eBottomLevel,
        geometry, primitiveCount);
    
}

void createTLAS(){
    vk::TransformMatrixKHR transform = std::array{
        std::array{1.0f, 0.0f, 0.0f, 0.0f},
        std::array{0.0f, 1.0f, 0.0f, 0.0f},
        std::array{0.0f, 0.0f, 1.0f, 0.0f},
    };

    vk::AccelerationStructureInstanceKHR accelInstance{};
    accelInstance.setTransform(transform);
    accelInstance.setInstanceCustomIndex(0);
    accelInstance.setMask(0xFF);
    accelInstance.setInstanceShaderBindingTableRecordOffset(0);
    accelInstance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);
    accelInstance.setAccelerationStructureReference(bottomAccel.buffer.address);

    Buffer instanceBuffer;
    instanceBuffer.init(
        physicalDevice, *device,
        sizeof(vk::AccelerationStructureInstanceKHR),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        &accelInstance);

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{};
    instancesData.setArrayOfPointers(false);
    instancesData.setData(instanceBuffer.address);

    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setGeometry({instancesData});
    geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    constexpr uint32_t primitiveCount = 1;
    topAccel.init(
        physicalDevice, *device, commandPool.get(), queue,
        vk::AccelerationStructureTypeKHR::eTopLevel,
        geometry, primitiveCount);
}