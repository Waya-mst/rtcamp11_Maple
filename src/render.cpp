#include "../include/globals.hpp"
#include "../include/vk_setup.hpp"
#include "../include/descriptors.hpp"
#include <iostream>
#include <cmath>
#include <chrono>
#include <stb_image_write.h>


void drawCall(){
    int frameIndex = 0;

    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    cmdAllocInfo.setCommandPool(commandPool.get());
    cmdAllocInfo.setCommandBufferCount(MAX_FRAMES);

    cmdBufs = device->allocateCommandBuffersUnique(cmdAllocInfo);

    vk::Extent2D swapchainExtent = surfaceCapabilities.currentExtent;

    std::array<vk::UniqueSemaphore, MAX_FRAMES> imageAvailable;
    std::vector<vk::UniqueSemaphore> renderFinished;
    std::array<vk::UniqueFence, MAX_FRAMES> inFlight;

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

    for(size_t i = 0; i < MAX_FRAMES; i++){
        imageAvailable[i] = device->createSemaphoreUnique(semaphoreCreateInfo);
        inFlight[i] = device->createFenceUnique(fenceCreateInfo);
    }
    renderFinished.resize(1);
    for (auto& s : renderFinished) {
        s = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
    }

    uint32_t currentFrame = 0;
    float time = 0;
    updateDescriptorSet(0, outputView.get());

    const auto start = std::chrono::system_clock::now();
    const auto deadline = start + std::chrono::seconds(80);

    int update = 0;
    
    while(frameIndex < 3 && std::chrono::system_clock::now() < deadline){
        {
            auto now = std::chrono::system_clock::now();
            auto remaining = (deadline > now) ? (deadline - now) : std::chrono::system_clock::duration::zero();
            auto slice = std::min(std::chrono::nanoseconds(50'000'000),
                                std::chrono::duration_cast<std::chrono::nanoseconds>(remaining));
            uint64_t ns_timeout = static_cast<uint64_t>(slice.count());
            auto waitRes = device->waitForFences(inFlight[currentFrame].get(), VK_TRUE, ns_timeout);
            if(waitRes == vk::Result::eTimeout){
                continue;
            }
        }
        auto waitRes = device->waitForFences(inFlight[currentFrame].get(), VK_TRUE, UINT64_MAX);
        device->resetFences(inFlight[0].get());

        //----------------------------------------------------------------------------
        // update uniformbuffer

        vk::DeviceSize bufferSize = sizeof(SceneUBO);

        static std::chrono::system_clock::time_point prevTime;
        static float up = 2.0f;

        const auto nowTime = std::chrono::system_clock::now();
        const auto delta = 0.05 * std::chrono::duration_cast<std::chrono::microseconds>(nowTime - prevTime).count();
        scene.camPos += up * std::sin(delta);
        memcpy(uniformData, &scene, (size_t)bufferSize);

        vk::MappedMemoryRange flushMemoryRange;
        flushMemoryRange.setMemory(sceneBuffer.memory.get());
        flushMemoryRange.setOffset(0);
        flushMemoryRange.setSize(VK_WHOLE_SIZE);
        device->flushMappedMemoryRanges({flushMemoryRange});

        // static float up = 0.2f;
        // scene.camPos += up * update;
        // memcpy(uniformData, &scene, (size_t)bufferSize);

        // vk::MappedMemoryRange flushMemoryRange;
        // flushMemoryRange.setMemory(sceneBuffer.memory.get());
        // flushMemoryRange.setOffset(0);
        // flushMemoryRange.setSize(VK_WHOLE_SIZE);
        // device->flushMappedMemoryRanges({flushMemoryRange});

        // update++;

        //----------------------------------------------------------------------------

        auto& cmdBuf = cmdBufs[0];
        cmdBuf->reset();

        vk::CommandBufferBeginInfo cmdBeginInfo{};
        cmdBuf->begin(cmdBeginInfo);

        vk::ImageSubresourceRange range{};
        range.aspectMask = vk::ImageAspectFlagBits::eColor;
        range.baseMipLevel = 0; range.levelCount  = 1;
        range.baseArrayLayer = 0; range.layerCount = 1;

        vk::ImageMemoryBarrier toGeneral{};
        toGeneral.oldLayout  = (frameIndex == 0)
                        ? vk::ImageLayout::eUndefined
                        : vk::ImageLayout::eTransferSrcOptimal;
        toGeneral.newLayout = vk::ImageLayout::eGeneral;
        toGeneral.srcAccessMask = (frameIndex == 0)
                        ? vk::AccessFlags{}
                        : vk::AccessFlagBits::eTransferRead;
        toGeneral.dstAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        toGeneral.image = outputImage.get();
        toGeneral.subresourceRange = range;

        vk::PipelineStageFlags srcStage =
            (frameIndex == 0) ? vk::PipelineStageFlagBits::eTopOfPipe
                            : vk::PipelineStageFlagBits::eTransfer;
        vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

        cmdBuf->pipelineBarrier(
            srcStage, dstStage,
            {}, nullptr, nullptr, toGeneral);

        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        cmdBuf->bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.get());
        cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout.get(), 0, {descSets[0].get()}, {});
        cmdBuf->traceRaysKHR(
            raygenRegion,
            missRegion,
            hitRegion,
            {},
            width, height, 1
        );
        
        vk::ImageMemoryBarrier toCopy{};
        toCopy.oldLayout = vk::ImageLayout::eGeneral;
        toCopy.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        toCopy.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        toCopy.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        toCopy.image = outputImage.get();
        toCopy.subresourceRange = range;

        cmdBuf->pipelineBarrier(
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::PipelineStageFlagBits::eTransfer,
            {}, nullptr, nullptr, toCopy);

        vk::BufferImageCopy copy{};
        copy.bufferOffset = 0;              // 先頭
        copy.bufferRowLength = 0;           // 0なら密詰め
        copy.bufferImageHeight = 0;         // 0なら密詰め
        copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = 1;
        copy.setImageOffset(vk::Offset3D{0, 0, 0});
        copy.setImageExtent({uint32_t(width), uint32_t{height}, 1});

        cmdBuf->copyImageToBuffer(
            outputImage.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            outputBuffer.buffer.get(),
            { copy }
        );

        vk::BufferMemoryBarrier bufBarrier{};
        bufBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        bufBarrier.dstAccessMask = vk::AccessFlagBits::eHostRead;
        bufBarrier.buffer = outputBuffer.buffer.get();
        bufBarrier.offset = 0;
        bufBarrier.size = VK_WHOLE_SIZE;

        cmdBuf->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eHost,
            {}, nullptr, bufBarrier, nullptr
        );

        cmdBuf->end();

        vk::CommandBuffer submitCmdBuf[1] = {cmdBuf.get()};
        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(submitCmdBuf);

        vk::PipelineStageFlags renderwaitStages[] = {vk::PipelineStageFlagBits::eRayTracingShaderKHR};
        submitInfo.setPWaitDstStageMask(renderwaitStages);
        
        queue.submit({submitInfo}, inFlight[0].get());

        auto fenceToWait = inFlight[0].get();

        waitRes = device->waitForFences(fenceToWait, VK_TRUE, UINT64_MAX);
        size_t size = size_t(width) * size_t(height) * 4;
        void* mapped = device->mapMemory(outputBuffer.memory.get(), 0, size);
        char filename[256];
        std::snprintf(filename, sizeof(filename), "%3u.png", frameIndex);
        stbi_write_png(filename, width, height, 4, mapped, int(width * 4));
        device->unmapMemory(outputBuffer.memory.get());

        frameIndex++;
        currentFrame = (currentFrame + 1) % MAX_FRAMES;
    }
    queue.waitIdle();
    return;
}