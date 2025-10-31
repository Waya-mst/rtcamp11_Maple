#include "../include/globals.hpp"
#include "../include/buffer.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>

void addShader(uint32_t shaderIndex,
                const std::string& filename,
                vk::ShaderStageFlagBits stage){
    size_t SpvFileSz = std::filesystem::file_size(filename);
    std::ifstream SpvFile(filename, std::ios_base::binary);

    std::vector<char> SpvFileData(SpvFileSz);
    SpvFile.read(SpvFileData.data(), SpvFileSz);

    vk::ShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.setCodeSize(SpvFileSz);
    shaderCreateInfo.setPCode(reinterpret_cast<const uint32_t*>(SpvFileData.data()));

    shaderModules[shaderIndex] = device->createShaderModuleUnique(shaderCreateInfo);
    shaderStages[shaderIndex].setStage(stage);
    shaderStages[shaderIndex].setModule(*shaderModules[shaderIndex]);
    shaderStages[shaderIndex].setPName("main");
}

void prepareShaders(){
    uint32_t raygenShader = 0;
    uint32_t missMainShader = 1;
    uint32_t missShadowShader = 2;
    uint32_t chitShader = 3;
    uint32_t visShader = 4;
    shaderStages.resize(5);
    shaderModules.resize(5);

    addShader(raygenShader, "build/shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR);
    addShader(missMainShader, "build/shaders/miss_main.spv", vk::ShaderStageFlagBits::eMissKHR);
    addShader(missShadowShader, "build/shaders/miss_shadow.spv", vk::ShaderStageFlagBits::eMissKHR);
    addShader(chitShader, "build/shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR);
    addShader(visShader, "build/shaders/anyhit.spv", vk::ShaderStageFlagBits::eAnyHitKHR);

    uint32_t raygenGroup = 0;
    uint32_t missMainGroup = 1;
    uint32_t missShadowGroup = 2;
    uint32_t hitGroup = 3;
    shaderGroups.resize(4);

    // Raygen group
    shaderGroups[raygenGroup].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    shaderGroups[raygenGroup].setGeneralShader(raygenShader);
    shaderGroups[raygenGroup].setClosestHitShader(VK_SHADER_UNUSED_KHR);
    shaderGroups[raygenGroup].setAnyHitShader(VK_SHADER_UNUSED_KHR);
    shaderGroups[raygenGroup].setIntersectionShader(VK_SHADER_UNUSED_KHR);

    // Miss_main group
    shaderGroups[missMainGroup].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    shaderGroups[missMainGroup].setGeneralShader(missMainShader);

    // Miss_shadow group
    shaderGroups[missShadowGroup].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    shaderGroups[missShadowGroup].setGeneralShader(missShadowShader);

    // Hit group
    shaderGroups[hitGroup].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
    shaderGroups[hitGroup].setGeneralShader(VK_SHADER_UNUSED_KHR);
    shaderGroups[hitGroup].setClosestHitShader(chitShader);
    shaderGroups[hitGroup].setAnyHitShader(visShader);
    shaderGroups[hitGroup].setIntersectionShader(VK_SHADER_UNUSED_KHR);
}

void createRayTracingPipeline(){
    vk::PipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.setSetLayouts(*descSetLayout);
    pipelineLayout = device->createPipelineLayoutUnique(layoutCreateInfo);

    vk::RayTracingPipelineCreateInfoKHR pipelineCreateInfo{};
    pipelineCreateInfo.setLayout(*pipelineLayout);
    pipelineCreateInfo.setStages(shaderStages);
    pipelineCreateInfo.setGroups(shaderGroups);
    pipelineCreateInfo.setMaxPipelineRayRecursionDepth(2);
    auto result = device->createRayTracingPipelineKHRUnique(nullptr, nullptr, pipelineCreateInfo);
    if(result.result != vk::Result::eSuccess){
        std::cerr << "Failed to create ray tracing pipeline.\n";
        std::abort();
    }
    pipeline = std::move(result.value);
}

void createShaderBindingTable(){
    auto deviceProps = physicalDevice.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps =
        deviceProps.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    
    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;
    uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);

    uint32_t raygenShaderCount = 1;
    uint32_t missShaderCount = 2;
    uint32_t hitShaderCount = 1;

    raygenRegion.setStride(alignUp(handleSizeAligned, baseAlignment));
    raygenRegion.setSize(raygenRegion.stride);

    missRegion.setStride(handleSizeAligned);
    missRegion.setSize(alignUp(missShaderCount * handleSizeAligned, baseAlignment));

    hitRegion.setStride(handleSizeAligned);
    hitRegion.setSize(alignUp(hitShaderCount * handleSizeAligned, baseAlignment));

    vk::DeviceSize sbtSize = raygenRegion.size + missRegion.size + hitRegion.size;
    sbt.init(physicalDevice, *device, sbtSize, 
            vk::BufferUsageFlagBits::eShaderBindingTableKHR |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    
    uint32_t handleCount = raygenShaderCount + missShaderCount + hitShaderCount;
    uint32_t handleStorageSize = handleCount * handleSize;
    std::vector<uint8_t> handleStorage(handleStorageSize);
    auto result = device->getRayTracingShaderGroupHandlesKHR(
        *pipeline, 0, handleCount, handleStorageSize, handleStorage.data());
    if (result != vk::Result::eSuccess) {
        std::cerr << "Failed to get ray tracing shader group handles.\n";
        std::abort();
    }

    uint8_t* sbtHead =
        static_cast<uint8_t*>(device->mapMemory(*sbt.memory, 0, sbtSize));

    uint8_t* dstPtr = sbtHead;
    auto copyHandle = [&](uint32_t index) {
        std::memcpy(dstPtr, handleStorage.data() + handleSize * index,
                    handleSize);
    };

    // Raygen
    uint32_t handleIndex = 0;
    copyHandle(handleIndex++);

    // Miss
    dstPtr = sbtHead + raygenRegion.size;
    for (uint32_t c = 0; c < missShaderCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += missRegion.stride;
    }

    // Hit
    dstPtr = sbtHead + raygenRegion.size + missRegion.size;
    for (uint32_t c = 0; c < hitShaderCount; c++) {
        copyHandle(handleIndex++);
        dstPtr += hitRegion.stride;
    }

    raygenRegion.setDeviceAddress(sbt.address);
    missRegion.setDeviceAddress(sbt.address + raygenRegion.size);
    hitRegion.setDeviceAddress(sbt.address + raygenRegion.size + missRegion.size);
}