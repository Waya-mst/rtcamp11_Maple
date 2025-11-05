#include "../include/globals.hpp"
#include "../include/buffer.hpp"
#include <glm/glm.hpp>
#include <iostream>

const std::string envMapPath = "./resource/envmap/env.hdr";
const std::string gltfPath = "./resource/mesh/test.gltf";

glm::vec3 CubemapDirectionFromFaceXY(int face, int x, int y, int faceSize)
{
    // [-1,1] に正規化したテクスチャ座標
    float s = ( (x + 0.5f) / (float)faceSize ) * 2.0f - 1.0f;
    float t = ( (y + 0.5f) / (float)faceSize ) * 2.0f - 1.0f;
    t = -t;

    glm::vec3 dir;
    switch (face)
    {
    case 0: // +X
        dir = glm::vec3( 1.0f,     t,    -s);
        break;
    case 1: // -X
        dir = glm::vec3(-1.0f,     t,     s);
        break;
    case 2: // +Y
        dir = glm::vec3(   s,  1.0f,    t);
        break;
    case 3: // -Y
        dir = glm::vec3(   s, -1.0f,   -t);
        break;
    case 4: // +Z
        dir = glm::vec3(   s,     t,  1.0f);
        break;
    case 5: // -Z
        dir = glm::vec3(  -s,     t, -1.0f);
        break;
    }
    return glm::normalize(dir);
}

glm::vec4 SampleEquirect(const glm::vec3& dir, const float* src, int w, int h)
{
    // dir -> (u,v)
    float u = std::atan2(dir.z, dir.x) / (2.0f * float(M_PI)) + 0.5f;
    float v = std::acos(std::clamp(dir.y, -1.0f, 1.0f)) / float(M_PI);

    int px = std::clamp(int(u * w), 0, w - 1);
    int py = std::clamp(int(v * h), 0, h - 1);

    const float* p = src + (py * w + px) * 4;
    return glm::vec4(p[0], p[1], p[2], p[3]);
}

void loadModel(){
    std::string err, warn;

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
            uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

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

            const auto getByteStride = [&](tinygltf::BufferView* bufferView, size_t defaultSize)->size_t{
                if(bufferView->byteStride > 0){
                    return bufferView->byteStride;
                }
                return defaultSize;
            };

            for(size_t i = 0; i < positionAccessor->count; i++){
                Vertex v{};
                {
                    size_t byteStride = getByteStride(positionBufferView, sizeof(glm::vec3));
                    size_t positionByteOffset = positionAccessor->byteOffset +
                                                positionBufferView->byteOffset + i * byteStride;
                    v.pos = *reinterpret_cast<const glm::vec3*>(
                        &(model.buffers[positionBufferView->buffer].data[positionByteOffset]));
                }

                if(normalBufferView){
                    size_t byteStride = getByteStride(normalBufferView, sizeof(glm::vec3));
                    size_t normalByteOffset = normalAccessor->byteOffset +
                                                normalBufferView->byteOffset + i * byteStride;
                    v.normal = *reinterpret_cast<const glm::vec3*>(
                        &(model.buffers[normalBufferView->buffer].data[normalByteOffset]));
                }
                if(texCoordBufferView){
                    size_t byteStride = getByteStride(texCoordBufferView, sizeof(glm::vec2));
                    size_t texCoordByteOffset = texCoordAccessor->byteOffset +
                                                texCoordBufferView->byteOffset + i * byteStride;
                    v.texCoord = *reinterpret_cast<const glm::vec2*>(
                        &(model.buffers[texCoordBufferView->buffer].data[texCoordByteOffset]));
                }
                vertices.push_back(v);
            }

            {
                auto& accessor   = model.accessors[gltfPrimitive.indices];
                size_t indexCount = accessor.count;
                size_t triCount   = indexCount / 3;

                uint32_t matId = (gltfPrimitive.material >= 0) ? (uint32_t)gltfPrimitive.material : 0u;

                for(size_t t = 0; t < triCount; ++t){
                    primitiveMaterialIndices.push_back(matId);
                }

                vk::BufferUsageFlags bufferUsage{vk::BufferUsageFlagBits::eStorageBuffer};
                vk::MemoryPropertyFlags memoryProperty{
                    vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent};

                materialIndexBuffer.init(
                    physicalDevice, *device, sizeof(uint32_t) * primitiveMaterialIndices.size(),
                    bufferUsage, memoryProperty, primitiveMaterialIndices.data());
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
                            indices.push_back(buf[i] + vertexOffset);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        uint16_t* buf = new uint16_t[indicesCount];
                        size_t size = indicesCount * sizeof(uint16_t);
                        std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                    size);
                        for (size_t i = 0; i < indicesCount; i++) {
                            indices.push_back(buf[i] + vertexOffset);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        uint8_t* buf = new uint8_t[indicesCount];
                        size_t size = indicesCount * sizeof(uint8_t);
                        std::memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
                                    size);
                        for (size_t i = 0; i < indicesCount; i++) {
                            indices.push_back(buf[i] + vertexOffset);
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
    textureImages.clear();
    textureMemorys.clear();
    textureImageViews.clear();

    std::string baseDir = gltfPath.substr(0, gltfPath.find_last_of("/\\") + 1);

    for(const auto& img : model.images){
        const unsigned char* data = nullptr;
        int width = 0;
        int height = 0;
        int comp = 4;
        bool needfree = false;

        if(!img.uri.empty()){
            std::string texPath = baseDir + "../texture/" + img.uri;
            int comp;
            auto loaded = stbi_load(texPath.c_str(), &width, &height, &comp, STBI_rgb_alpha);
            if(loaded == nullptr) {
                std::cerr << "画像ファイルの読み込みに失敗しました。" << std::endl;
                return;
            }
            data = loaded;
            needfree = true;
        }else if(img.bufferView >= 0){
            data = img.image.data();
            width = img.width;
            height = img.height;
            comp = img.component;
        }

        size_t imgDataSize = 4 * width * height;

        Buffer buffer;
        buffer.init(
            physicalDevice, *device, imgDataSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            data);
        textureBuffers.push_back(std::move(buffer));

        if(needfree) stbi_image_free((void*)data);

        vk::ImageCreateInfo imgCI{};
        imgCI.setImageType(vk::ImageType::e2D);
        imgCI.setExtent(vk::Extent3D{(uint32_t)width, (uint32_t)height, 1});
        imgCI.setMipLevels(1); imgCI.setArrayLayers(1);
        imgCI.setFormat(vk::Format::eR8G8B8A8Unorm);
        imgCI.setTiling(vk::ImageTiling::eOptimal);
        imgCI.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
        imgCI.setSamples(vk::SampleCountFlagBits::e1);
        imgCI.setSharingMode(vk::SharingMode::eExclusive);

        auto image = device->createImageUnique(imgCI);
        auto memReq = device->getImageMemoryRequirements(image.get());
        uint32_t memIndex = 0;
        for(uint32_t i = 0; i < physicalDevice.getMemoryProperties().memoryTypeCount; i++){
            if((memReq.memoryTypeBits & (1 << i)) &&
            (physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
            {
                memIndex = i; break;
            }
        }

        vk::MemoryAllocateInfo mAI{memReq.size, memIndex};
        auto mem = device->allocateMemoryUnique(mAI);
        device->bindImageMemory(image.get(), mem.get(), 0);

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
            barrior.image = image.get();
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
            imgCopyRegion.setImageExtent({uint32_t(width), uint32_t(height), 1});
            imgCopyRegion.setBufferRowLength(0);
            imgCopyRegion.setBufferImageHeight(0);

            auto& staging = textureBuffers.back();
            tmpCmdBufs[0]->copyBufferToImage(
                staging.buffer.get(),
                image.get(), vk::ImageLayout::eTransferDstOptimal, { imgCopyRegion });
        }
        {
            vk::ImageMemoryBarrier barrior;
            barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrior.image = image.get();
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

        vk::ImageViewCreateInfo texImgViewCreateInfo;
        texImgViewCreateInfo.image = image.get();
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
        vk::UniqueImageView texImgView = device->createImageViewUnique(texImgViewCreateInfo);

        textureImages.push_back(std::move(image));
        textureMemorys.push_back(std::move(mem));
        textureImageViews.push_back(std::move(texImgView));
    }

    int envWidth, envHeight, envCh;
    uint32_t faceSize = 2048;
    size_t   facePixels = (size_t)faceSize * (size_t)faceSize;
    size_t   faceBytes  = facePixels * 4 * sizeof(float);   // RGBA32F
    size_t   totalBytes = faceBytes * 6;

    auto pEnvData = stbi_loadf(envMapPath.c_str(), &envWidth, &envHeight, &envCh, STBI_rgb_alpha);

    if(pEnvData == nullptr) {
        std::cerr << "画像ファイルの読み込みに失敗しました。" << std::endl;
        return;
    }

    size_t envDataSize = 4 * envWidth * envHeight * sizeof(float);

    vk::ImageCreateInfo envCI{};
    envCI.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
    envCI.setImageType(vk::ImageType::e2D);
    envCI.setExtent({(uint32_t)faceSize, (uint32_t)faceSize, 1});
    envCI.setMipLevels(1); envCI.setArrayLayers(6);
    envCI.setFormat(vk::Format::eR32G32B32A32Sfloat);
    envCI.setTiling(vk::ImageTiling::eOptimal);
    envCI.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
    envCI.setSharingMode(vk::SharingMode::eExclusive);
    envCI.setSamples(vk::SampleCountFlagBits::e1);

    envTexImage = device->createImageUnique(envCI);

    auto memReq = device->getImageMemoryRequirements(envTexImage.get());
    uint32_t memIndex = 0;
    for(uint32_t i = 0; i < physicalDevice.getMemoryProperties().memoryTypeCount; i++){
        if((memReq.memoryTypeBits & (1 << i)) &&
        (physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            memIndex = i; break;
        }
    }
    vk::MemoryAllocateInfo envMAI{memReq.size, memIndex};
    envTexMemory = device->allocateMemoryUnique(envMAI);
    device->bindImageMemory(envTexImage.get(), envTexMemory.get(), 0);

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
        barrior.image = envTexImage.get();
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 6;
        barrior.srcAccessMask = {};
        barrior.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        tmpCmdBufs[0]->pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrior});
    }
    {
        std::vector<float> cube(6ull * facePixels * 4);

        for (int face = 0; face < 6; ++face) {
            for (int y = 0; y < faceSize; ++y) {
                for (int x = 0; x < faceSize; ++x) {
                    glm::vec3 dir = CubemapDirectionFromFaceXY(face, x, y, faceSize);
                    glm::vec4 c   = SampleEquirect(dir, pEnvData, envWidth, envHeight);
                    size_t idx = ( (size_t)face * facePixels + y * faceSize + x ) * 4;
                    cube[idx + 0] = c.r;
                    cube[idx + 1] = c.g;
                    cube[idx + 2] = c.b;
                    cube[idx + 3] = c.a;
                }
            }
        }
        envTexBuffers[0].init(
            physicalDevice, *device,
            totalBytes,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            cube.data()
        );

        std::vector<vk::BufferImageCopy> copies;
        copies.reserve(6);

        for (uint32_t face = 0; face < 6; ++face)
        {
            vk::BufferImageCopy copy{};
            copy.bufferOffset = faceBytes * face;
            copy.bufferRowLength   = 0;
            copy.bufferImageHeight = 0;

            copy.imageSubresource.aspectMask     = vk::ImageAspectFlagBits::eColor;
            copy.imageSubresource.mipLevel       = 0;
            copy.imageSubresource.baseArrayLayer = face;
            copy.imageSubresource.layerCount     = 1;

            copy.imageOffset = vk::Offset3D{0, 0, 0};
            copy.imageExtent = vk::Extent3D{
                (uint32_t)faceSize,
                (uint32_t)faceSize,
                1
            };

            copies.push_back(copy);
        }
        tmpCmdBufs[0]->copyBufferToImage(
            envTexBuffers[0].buffer.get(),
            envTexImage.get(), vk::ImageLayout::eTransferDstOptimal, copies);
    }
    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = envTexImage.get();
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 6;
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
    envSampler = device->createSamplerUnique(samplerCreateInfo);

    vk::ImageViewCreateInfo envTexImageViewCI;
    envTexImageViewCI.image = envTexImage.get();
    envTexImageViewCI.viewType = vk::ImageViewType::eCube;
    envTexImageViewCI.format = vk::Format::eR32G32B32A32Sfloat;
    envTexImageViewCI.components.r = vk::ComponentSwizzle::eIdentity;
    envTexImageViewCI.components.g = vk::ComponentSwizzle::eIdentity;
    envTexImageViewCI.components.b = vk::ComponentSwizzle::eIdentity;
    envTexImageViewCI.components.a = vk::ComponentSwizzle::eIdentity;
    envTexImageViewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    envTexImageViewCI.subresourceRange.baseMipLevel = 0;
    envTexImageViewCI.subresourceRange.levelCount = 1;
    envTexImageViewCI.subresourceRange.baseArrayLayer = 0;
    envTexImageViewCI.subresourceRange.layerCount = 6;
    envImageView = device->createImageViewUnique(envTexImageViewCI);
}

void loadMaterial(){
    materials.clear();
    materials.reserve(model.materials.size());

    for(const auto& mat : model.materials){
        Material m;
        const auto& pbr = mat.pbrMetallicRoughness;

        if (pbr.baseColorFactor.size() == 4) {
            m.baseColorFactor = glm::vec4(
                (float)pbr.baseColorFactor[0],
                (float)pbr.baseColorFactor[1],
                (float)pbr.baseColorFactor[2],
                (float)pbr.baseColorFactor[3]
            );
        } else {
            m.baseColorFactor = glm::vec4(1.0f);
        }

        m.metallicFactor  = (float)pbr.metallicFactor;
        m.roughnessFactor = (float)pbr.roughnessFactor;

        if (pbr.baseColorTexture.index >= 0) {
            m.baseColorTextureIndex = pbr.baseColorTexture.index;
        }

        if (pbr.metallicRoughnessTexture.index >= 0) {
            m.matallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
        }

        if (mat.normalTexture.index >= 0) {
            m.normalTextureIndex = mat.normalTexture.index;
        }

        if (mat.occlusionTexture.index >= 0) {
            m.occulusionTextureIndex = mat.occlusionTexture.index;
        }

        if (mat.emissiveTexture.index >= 0) {
            m.emissiveTextureIndex = mat.emissiveTexture.index;
        }

        if (mat.emissiveFactor.size() == 3) {
            m.emissiveFactor = glm::vec4(
                (float)mat.emissiveFactor[0],
                (float)mat.emissiveFactor[1],
                (float)mat.emissiveFactor[2],
                1.0f
            );
        } else {
            m.emissiveFactor = glm::vec4(0.0f);
        }

        m.ior = 1.5f;
        auto extIor = mat.extensions.find("KHR_materials_ior");
        if (extIor != mat.extensions.end()) {
            const tinygltf::Value& ext = extIor->second;
            if (ext.Has("ior")) {
                m.ior = (float)ext.Get("ior").GetNumberAsDouble();
            }
        }

        materials.push_back(m);
    }

    vk::BufferUsageFlags bufferUsage{vk::BufferUsageFlagBits::eStorageBuffer};
    vk::MemoryPropertyFlags memoryProperty{
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent};

    materialBuffer.init(
        physicalDevice, *device, sizeof(Material) * materials.size(),
        bufferUsage, memoryProperty, materials.data());
}