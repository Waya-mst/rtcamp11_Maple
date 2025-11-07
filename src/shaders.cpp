#include "../include/globals.hpp"
#include "../include/buffer.hpp"

#include "raygen_spv.hpp"
#include "miss_main_spv.hpp"
#include "miss_shadow_spv.hpp"
#include "closesthit_spv.hpp"
#include "anyhit_spv.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>

vk::UniqueShaderModule createShaderModuleFromEmbedded(vk::Device &device,
                                              const void* data,
                                              size_t sizeBytes)
{
    vk::ShaderModuleCreateInfo ci{};
    ci.codeSize = sizeBytes;
    ci.pCode    = static_cast<const uint32_t*>(data);  // SPIR-Vなのでuint32_t*
    return device.createShaderModuleUnique(ci);
}

void prepareShaders(){
    uint32_t raygenShader = 0;
    uint32_t missMainShader = 1;
    uint32_t missShadowShader = 2;
    uint32_t chitShader = 3;
    uint32_t visShader = 4;
    shaderStages.resize(5);
    shaderModules.resize(5);

    // 1) Raygen
    {
        auto mod = createShaderModuleFromEmbedded(*device, raygen_spv, raygen_spv_size);
        shaderModules[raygenShader] = std::move(mod);
        shaderStages[raygenShader].setStage(vk::ShaderStageFlagBits::eRaygenKHR);
        shaderStages[raygenShader].setModule(*shaderModules[raygenShader]);
        shaderStages[raygenShader].setPName("main");   // エントリ名は今までと同じ
    }

    // 2) Miss (main)
    {
        auto mod = createShaderModuleFromEmbedded(*device, miss_main_spv, miss_main_spv_size);
        shaderModules[missMainShader] = std::move(mod);
        shaderStages[missMainShader].setStage(vk::ShaderStageFlagBits::eMissKHR);
        shaderStages[missMainShader].setModule(*shaderModules[missMainShader]);
        shaderStages[missMainShader].setPName("main");
    }

    // 3) Miss (shadow)
    {
        auto mod = createShaderModuleFromEmbedded(*device, miss_shadow_spv, miss_shadow_spv_size);
        shaderModules[missShadowShader] = std::move(mod);
        shaderStages[missShadowShader].setStage(vk::ShaderStageFlagBits::eMissKHR);
        shaderStages[missShadowShader].setModule(*shaderModules[missShadowShader]);
        shaderStages[missShadowShader].setPName("main");
    }

    // 4) Closest hit
    {
        auto mod = createShaderModuleFromEmbedded(*device, closesthit_spv, closesthit_spv_size);
        shaderModules[chitShader] = std::move(mod);
        shaderStages[chitShader].setStage(vk::ShaderStageFlagBits::eClosestHitKHR);
        shaderStages[chitShader].setModule(*shaderModules[chitShader]);
        shaderStages[chitShader].setPName("main");
    }

    // 5) Any hit
    {
        auto mod = createShaderModuleFromEmbedded(*device, anyhit_spv, anyhit_spv_size);
        shaderModules[visShader] = std::move(mod);
        shaderStages[visShader].setStage(vk::ShaderStageFlagBits::eAnyHitKHR);
        shaderStages[visShader].setModule(*shaderModules[visShader]);
        shaderStages[visShader].setPName("main");
    }

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