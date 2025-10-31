#include "../include/globals.hpp"
#include "../include/buffer.hpp"
#include <iostream>

void loadModel(){
    std::string err, warn;
    const std::string gltfPath = "./resource/mesh/test.gltf";

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);
    if(!warn.empty()) std::cerr << "[tinygltf warn] " << warn << "\n";
    if(!ret){
        std::cerr << "[tinygltf err] " << err << "\n";
        std::cerr << "Failed to load: " << gltfPath << "\n";
        std::cerr << "Working dir: " << std::filesystem::current_path().string() << "\n";
        std::abort();
    }

    size_t meshCount = 0;
    for (size_t gltfMeshIndex = 0; gltfMeshIndex < model.meshes.size(); gltfMeshIndex++) {
        auto& gltfMesh = model.meshes.at(gltfMeshIndex);
        meshCount += gltfMesh.primitives.size();
    }

    size_t meshIndex = 0;
    for(int gltfMeshIndex = 0; gltfMeshIndex < model.meshes.size(); gltfMeshIndex++){
        auto& gltfMesh = model.meshes.at(gltfMeshIndex);
        for(const auto& gltfPrimitive : gltfMesh.primitives){
            // Vertex Attributes
            auto& attributes = gltfPrimitive.attributes;

            assert(attributes.find("POSITION") != attributes.end());
            int positionIndex = attributes.find("POSITION")->second;
            tinygltf::Accessor* positionAccessor = &model.accessors[positionIndex];
            tinygltf::BufferView* positionBufferView = &model.bufferViews[positionAccessor->bufferView];

            tinygltf::Accessor* normalAccessor = nullptr;
            tinygltf::BufferView* normalBufferView = nullptr;
            if(attributes.find("NORMAL") != attributes.end()){
                int normalIndex = attributes.find("NORMAL")->second;
                normalAccessor = &model.accessors[normalIndex];
                normalBufferView = &model.bufferViews[normalAccessor->bufferView];
            }

            tinygltf::Accessor* texCoordAccessor = nullptr;
            tinygltf::BufferView* texCoordBufferView = nullptr;
            if(attributes.find("TEXCOORD_0") != attributes.end()){
                int texCoordIndex = attributes.find("TEXCOORD_0")->second;
                texCoordAccessor = &model.accessors[texCoordIndex];
                texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
            }

            vertices.resize(positionAccessor->count);

            const auto getByteStride = [&](tinygltf::BufferView* bufferView, size_t defaultSize)->size_t{
                if(bufferView->byteStride > 0){
                    return bufferView->byteStride;
                }
                return defaultSize;
            };

            for(size_t i = 0; i < positionAccessor->count; i++){
                {
                    size_t byteStride = getByteStride(positionBufferView, sizeof(glm::vec3));
                    size_t positionByteOffset = positionAccessor->byteOffset +
                                                positionBufferView->byteOffset + i * byteStride;
                    vertices[i].pos = *reinterpret_cast<const glm::vec3*>(
                        &(model.buffers[positionBufferView->buffer].data[positionByteOffset]));
                }

                if(normalBufferView){
                    size_t byteStride = getByteStride(normalBufferView, sizeof(glm::vec3));
                    size_t normalByteOffset = normalAccessor->byteOffset +
                                                normalBufferView->byteOffset + i * byteStride;
                    vertices[i].normal = *reinterpret_cast<const glm::vec3*>(
                        &(model.buffers[normalBufferView->buffer].data[normalByteOffset]));
                }
                if(texCoordBufferView){
                    size_t byteStride = getByteStride(texCoordBufferView, sizeof(glm::vec2));
                    size_t texCoordByteOffset = texCoordAccessor->byteOffset +
                                                texCoordBufferView->byteOffset + i * byteStride;
                    vertices[i].texCoord = *reinterpret_cast<const glm::vec2*>(
                        &(model.buffers[texCoordBufferView->buffer].data[texCoordByteOffset]));
                }
            }

            {
                auto& accessor = model.accessors[gltfPrimitive.indices];
                auto& bufferView = model.bufferViews[accessor.bufferView];
                auto& buffer = model.buffers[bufferView.buffer];

                size_t indicesCount = accessor.count;
                switch(accessor.componentType){
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        uint32_t* buf = new uint32_t[indicesCount];
                        size_t size = indicesCount * sizeof(uint32_t);
                        std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                    size);
                        for (size_t i = 0; i < indicesCount; i++) {
                            indices.push_back(buf[i]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        uint16_t* buf = new uint16_t[indicesCount];
                        size_t size = indicesCount * sizeof(uint16_t);
                        std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                    size);
                        for (size_t i = 0; i < indicesCount; i++) {
                            indices.push_back(buf[i]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        uint8_t* buf = new uint8_t[indicesCount];
                        size_t size = indicesCount * sizeof(uint8_t);
                        std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                    size);
                        for (size_t i = 0; i < indicesCount; i++) {
                            indices.push_back(buf[i]);
                        }
                        break;
                    }
                    default:
                        return;
                }
            }
        }
    }
}

void loadTexture(){
    int imgWidth, imgHeight, imgCh;
    const std::string texturePath = "./resource/texture/wood.jpg";
    auto pImgData = stbi_load(texturePath.c_str(), &imgWidth, &imgHeight, &imgCh, STBI_rgb_alpha);
    if(pImgData == nullptr) {
        std::cerr << "画像ファイルの読み込みに失敗しました。" << std::endl;
        return;
    }
    size_t imgDataSize = 4 * imgWidth * imgHeight;
    textureBuffers[0].init(
        physicalDevice, *device, imgDataSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        pImgData);

    stbi_image_free(pImgData);

    vk::ImageCreateInfo imgCI{};
    imgCI.setImageType(vk::ImageType::e2D);
    imgCI.setExtent({(uint32_t)imgWidth, (uint32_t)imgHeight, 1});
    imgCI.setMipLevels(1); imgCI.setArrayLayers(1);
    imgCI.setFormat(vk::Format::eR8G8B8A8Unorm);
    imgCI.setTiling(vk::ImageTiling::eOptimal);
    imgCI.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
    imgCI.setSharingMode(vk::SharingMode::eExclusive);
    imgCI.setSamples(vk::SampleCountFlagBits::e1);

    textureImage = device->createImageUnique(imgCI);

    auto memReq = device->getImageMemoryRequirements(textureImage.get());
    uint32_t memIndex = 0;
    for(uint32_t i = 0; i < physicalDevice.getMemoryProperties().memoryTypeCount; i++){
        if((memReq.memoryTypeBits & (1 << i)) &&
        (physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            memIndex = i; break;
        }
    }
    vk::MemoryAllocateInfo mAI{memReq.size, memIndex};
    textureMemory = device->allocateMemoryUnique(mAI);
    device->bindImageMemory(textureImage.get(), textureMemory.get(), 0);

    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = commandPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    tmpCmdBufs[0]->begin(cmdBeginInfo);

    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eUndefined;
        barrior.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = textureImage.get();
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        barrior.srcAccessMask = {};
        barrior.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        tmpCmdBufs[0]->pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrior});
    }
    {
        vk::BufferImageCopy imgCopyRegion;
        imgCopyRegion.setBufferOffset(0);
        imgCopyRegion.imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        imgCopyRegion.imageSubresource.setMipLevel(0);
        imgCopyRegion.imageSubresource.setBaseArrayLayer(0);
        imgCopyRegion.imageSubresource.setLayerCount(1);
        imgCopyRegion.setImageOffset(vk::Offset3D{0, 0, 0});
        imgCopyRegion.setImageExtent({uint32_t(imgWidth), uint32_t(imgHeight), 1});
        imgCopyRegion.setBufferRowLength(0);
        imgCopyRegion.setBufferImageHeight(0);

        tmpCmdBufs[0]->copyBufferToImage(
            textureBuffers[0].buffer.get(),
            textureImage.get(), vk::ImageLayout::eTransferDstOptimal, { imgCopyRegion });
    }
    {
        
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = textureImage.get();
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        barrior.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrior.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        tmpCmdBufs[0]->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR, {}, {}, {}, {barrior});
    }

    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1);
    submitInfo.setPCommandBuffers(submitCmdBuf);

    queue.submit({submitInfo});
    queue.waitIdle();

    vk::SamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.anisotropyEnable = false;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = false;
    samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    sampler = device->createSamplerUnique(samplerCreateInfo);

    vk::ImageViewCreateInfo texImgViewCreateInfo;
    texImgViewCreateInfo.image = textureImage.get();
    texImgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    texImgViewCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    texImgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    texImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    texImgViewCreateInfo.subresourceRange.levelCount = 1;
    texImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    texImgViewCreateInfo.subresourceRange.layerCount = 1;
    textureImageView = device->createImageViewUnique(texImgViewCreateInfo);
}