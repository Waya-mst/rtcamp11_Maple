#include "../include/accel.hpp"
#include "../include/globals.hpp"

void createDescriptor(size_t countSets){
    size_t imageCount = std::max<size_t>(1, model.images.size());

    const uint32_t asPerSet      = 1;
    const uint32_t imgPerSet     = 1;
    const uint32_t uboPerSet     = 1;
    const uint32_t ssboPerSet    = 4;
    const uint32_t texPerSet     = 1 + imageCount; // envMap + texture
    const uint32_t samplerPerset = 2;

    std::vector<vk::DescriptorPoolSize> poolSizes;
    if (asPerSet)
        poolSizes.push_back({ vk::DescriptorType::eAccelerationStructureKHR, uint32_t(asPerSet * countSets) });
    if (imgPerSet)
        poolSizes.push_back({ vk::DescriptorType::eStorageImage,             uint32_t(imgPerSet * countSets) });
    if (uboPerSet)
        poolSizes.push_back({ vk::DescriptorType::eUniformBuffer,            uint32_t(uboPerSet * countSets) });
    if (ssboPerSet)
        poolSizes.push_back({ vk::DescriptorType::eStorageBuffer,            uint32_t(ssboPerSet * countSets) });
    if (texPerSet)
        poolSizes.push_back({ vk::DescriptorType::eSampledImage,             uint32_t(texPerSet * countSets) });
    if(samplerPerset)
        poolSizes.push_back({ vk::DescriptorType::eSampler,                  uint32_t(samplerPerset * countSets)});

    vk::DescriptorPoolCreateInfo createInfo{};
    createInfo.setPoolSizes(poolSizes);
    createInfo.setMaxSets(countSets);
    createInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    descPool = device->createDescriptorPoolUnique(createInfo);

    std::vector<vk::DescriptorSetLayoutBinding> bindings(11);

    // Acceleration Structure
    bindings[0].setBinding(0);
    bindings[0].setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
    bindings[0].setDescriptorCount(1);
    bindings[0].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    // Output Image
    bindings[1].setBinding(1);
    bindings[1].setDescriptorType(vk::DescriptorType::eStorageImage);
    bindings[1].setDescriptorCount(1);
    bindings[1].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    // Uniform Buffer(scene)
    bindings[2].setBinding(2);
    bindings[2].setDescriptorType(vk::DescriptorType::eUniformBuffer);
    bindings[2].setDescriptorCount(1);
    bindings[2].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);

    // vertex buffer
    bindings[3].setBinding(3);
    bindings[3].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    bindings[3].setDescriptorCount(1);
    bindings[3].setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    // index buffer
    bindings[4].setBinding(4);
    bindings[4].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    bindings[4].setDescriptorCount(1);
    bindings[4].setStageFlags(vk::ShaderStageFlagBits::eClosestHitKHR);

    // material buffer
    bindings[5].setBinding(5);
    bindings[5].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    bindings[5].setDescriptorCount(1);
    bindings[5].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);

    // material index buffer
    bindings[6].setBinding(6);
    bindings[6].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    bindings[6].setDescriptorCount(1);
    bindings[6].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);

    // sampled Image
    bindings[7].setBinding(7);
    bindings[7].setDescriptorType(vk::DescriptorType::eSampledImage);
    bindings[7].setDescriptorCount(textureImageViews.size());
    bindings[7].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    // sampler
    bindings[8].setBinding(8);
    bindings[8].setDescriptorType(vk::DescriptorType::eSampler);
    bindings[8].setDescriptorCount(1);
    bindings[8].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    // env Map
    bindings[9].setBinding(9);
    bindings[9].setDescriptorType(vk::DescriptorType::eSampledImage);
    bindings[9].setDescriptorCount(1);
    bindings[9].setStageFlags(
        vk::ShaderStageFlagBits::eRaygenKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR |
        vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eAnyHitKHR
    );

    // envMap sampler
    bindings[10].setBinding(10);
    bindings[10].setDescriptorType(vk::DescriptorType::eSampler);
    bindings[10].setDescriptorCount(1);
    bindings[10].setStageFlags(
        vk::ShaderStageFlagBits::eRaygenKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR |
        vk::ShaderStageFlagBits::eMissKHR |
        vk::ShaderStageFlagBits::eAnyHitKHR
    );


    vk::DescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
    descSetLayoutCreateInfo.setBindings(bindings);
    descSetLayout = device->createDescriptorSetLayoutUnique(descSetLayoutCreateInfo);

    std::vector<vk::DescriptorSetLayout> layouts(countSets, *descSetLayout);

    vk::DescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.setDescriptorPool(*descPool);
    allocateInfo.setSetLayouts(layouts);
    descSets = std::move(device->allocateDescriptorSetsUnique(allocateInfo));
}

void updateDescriptorSet(uint32_t setIndex, vk::ImageView imageView){
    std::vector<vk::WriteDescriptorSet> writes(11);

    // [0]: For AS
    vk::WriteDescriptorSetAccelerationStructureKHR accelInfo{};
    accelInfo.setAccelerationStructures(*topAccel.accel);
    writes[0].setDstSet(*descSets[setIndex]);
    writes[0].setDstBinding(0);
    writes[0].setDescriptorCount(1);
    writes[0].setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
    writes[0].setPNext(&accelInfo);

    // [1]: For storage image
    vk::DescriptorImageInfo imageInfo{};
    imageInfo.setImageView(imageView);
    imageInfo.setImageLayout(vk::ImageLayout::eGeneral);
    writes[1].setDstSet(*descSets[setIndex]);
    writes[1].setDstBinding(1);
    writes[1].setDescriptorType(vk::DescriptorType::eStorageImage);
    writes[1].setImageInfo(imageInfo);

    // [2]: For Light
    vk::DescriptorBufferInfo uboInfo{};
    uboInfo.setBuffer(sceneBuffer.buffer.get());
    uboInfo.setOffset(0);
    uboInfo.setRange(sizeof(SceneUBO));
    writes[2].setDstSet(*descSets[setIndex]);
    writes[2].setDstBinding(2);
    writes[2].setDescriptorType(vk::DescriptorType::eUniformBuffer);
    writes[2].setBufferInfo(uboInfo);

    // [3]: For vertexBuffer
    vk::DescriptorBufferInfo vinfo{};
    vinfo.setBuffer(vertexBuffer.buffer.get());
    vinfo.setOffset(0);
    vinfo.setRange(VK_WHOLE_SIZE);
    writes[3].setDstSet(*descSets[setIndex]);
    writes[3].setDstBinding(3);
    writes[3].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    writes[3].setBufferInfo(vinfo);

    // [4]: For indexBuffer
    vk::DescriptorBufferInfo idxinfo{};
    idxinfo.setBuffer(indexBuffer.buffer.get());
    idxinfo.setOffset(0);
    idxinfo.setRange(VK_WHOLE_SIZE);
    writes[4].setDstSet(*descSets[setIndex]);
    writes[4].setDstBinding(4);
    writes[4].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    writes[4].setBufferInfo(idxinfo);

    // [5]: For materialBuffer
    vk::DescriptorBufferInfo matBufInfo{};
    matBufInfo.setBuffer(materialBuffer.buffer.get());
    matBufInfo.setOffset(0);
    matBufInfo.setRange(sizeof(Material) * materials.size());
    writes[5].setDstSet(*descSets[setIndex]);
    writes[5].setDstBinding(5);
    writes[5].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    writes[5].setBufferInfo(matBufInfo);

    // [6]: For material index buffer
    vk::DescriptorBufferInfo matIdxInfo{};
    matIdxInfo.setBuffer(materialIndexBuffer.buffer.get());
    matIdxInfo.setOffset(0);
    matIdxInfo.setRange(sizeof(uint32_t) * primitiveMaterialIndices.size());
    writes[6].setDstSet(*descSets[setIndex]);
    writes[6].setDstBinding(6);
    writes[6].setDescriptorType(vk::DescriptorType::eStorageBuffer);
    writes[6].setBufferInfo(matIdxInfo);

    // [7]: For texture
    std::vector<vk::DescriptorImageInfo> texInfos;
    for(auto& v : textureImageViews){
        vk::DescriptorImageInfo iI;
        iI.setImageView(v.get());
        iI.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        texInfos.push_back(iI);
    }
    writes[7].setDstSet(*descSets[setIndex]);
    writes[7].setDstBinding(7);
    writes[7].setDescriptorCount((uint32_t)texInfos.size());
    writes[7].setDescriptorType(vk::DescriptorType::eSampledImage);
    writes[7].setPImageInfo(texInfos.data());

    // [8]: For sampler
    vk::DescriptorImageInfo samplerinfo[1];
    samplerinfo[0].setSampler(sampler.get());
    writes[8].setDstSet(*descSets[setIndex]);
    writes[8].setDstBinding(8);
    writes[8].setDescriptorCount(1);
    writes[8].setDescriptorType(vk::DescriptorType::eSampler);
    writes[8].setPImageInfo(samplerinfo);

    // [9]: For envMap
    vk::DescriptorImageInfo envTexImginfo[1];
    envTexImginfo[0].setImageView(envImageView.get());
    envTexImginfo[0].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    writes[9].setDstSet(*descSets[setIndex]);
    writes[9].setDstBinding(9);
    writes[9].setDescriptorCount(1);
    writes[9].setDescriptorType(vk::DescriptorType::eSampledImage);
    writes[9].setPImageInfo(envTexImginfo);

    // [10]: For envMap sampler
    vk::DescriptorImageInfo envSamplerinfo[1];
    envSamplerinfo[0].setSampler(envSampler.get());
    writes[10].setDstSet(*descSets[setIndex]);
    writes[10].setDstBinding(10);
    writes[10].setDescriptorCount(1);
    writes[10].setDescriptorType(vk::DescriptorType::eSampler);
    writes[10].setPImageInfo(envSamplerinfo);

    // Update
    device->updateDescriptorSets(writes, nullptr);
}