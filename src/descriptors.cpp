#include "../include/accel.hpp"
#include "../include/globals.hpp"

void createDescriptor(size_t countSets){
    const uint32_t asPerSet      = 1;
    const uint32_t imgPerSet     = 1;
    const uint32_t uboPerSet     = 1;
    const uint32_t ssboPerSet    = 3;
    const uint32_t texPerSet     = 1;
    const uint32_t samplerPerset = 1;

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

    std::vector<vk::DescriptorSetLayoutBinding> bindings(7);

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

    // sampled Image
    bindings[5].setBinding(5);
    bindings[5].setDescriptorType(vk::DescriptorType::eSampledImage);
    bindings[5].setDescriptorCount(1);
    bindings[5].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

    // sampler
    bindings[6].setBinding(6);
    bindings[6].setDescriptorType(vk::DescriptorType::eSampler);
    bindings[6].setDescriptorCount(1);
    bindings[6].setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);

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
    std::vector<vk::WriteDescriptorSet> writes(7);

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

    // [5]: For texture
    vk::DescriptorImageInfo texImginfo[1];
    texImginfo[0].setImageView(textureImageView.get());
    texImginfo[0].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    writes[5].setDstSet(*descSets[setIndex]);
    writes[5].setDstBinding(5);
    writes[5].setDescriptorCount(1);
    writes[5].setDescriptorType(vk::DescriptorType::eSampledImage);
    writes[5].setPImageInfo(texImginfo);

    // [6]: For sampler
    vk::DescriptorImageInfo samplerinfo[1];
    samplerinfo[0].setSampler(sampler.get());
    writes[6].setDstSet(*descSets[setIndex]);
    writes[6].setDstBinding(6);
    writes[6].setDescriptorCount(1);
    writes[6].setDescriptorType(vk::DescriptorType::eSampler);
    writes[6].setPImageInfo(samplerinfo);

    // Update
    device->updateDescriptorSets(writes, nullptr);
}