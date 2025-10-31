#include "../include/globals.hpp"
#include "../include/vk_setup.hpp"
#include "../include/descriptors.hpp"
#include <iostream>

void drawCall(){
    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    cmdAllocInfo.setCommandPool(commandPool.get());
    cmdAllocInfo.setCommandBufferCount(MAX_FRAMES);

    cmdBufs = device->allocateCommandBuffersUnique(cmdAllocInfo);

    vk::Extent2D swapchainExtent = surfaceCapabilities.currentExtent;

    std::array<vk::UniqueSemaphore, MAX_FRAMES> imageAvailable;
    std::vector<vk::UniqueSemaphore> renderFinished;
    std::array<vk::UniqueFence, MAX_FRAMES> inFlight;
    std::vector<vk::Fence> imagesInFlight(swapchainImages.size(), vk::Fence{});

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

    for(size_t i = 0; i < MAX_FRAMES; i++){
        imageAvailable[i] = device->createSemaphoreUnique(semaphoreCreateInfo);
        inFlight[i] = device->createFenceUnique(fenceCreateInfo);
    }
    renderFinished.resize(swapchainImages.size());
    for (auto& s : renderFinished) {
        s = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
    }

    uint32_t currentFrame = 0;
    float time = 0;

    // 流れ
    // acquireNextImageKHRでイメージの取得
    // swapchainImgSemaphoreにシグナル
    // swapchainImgSemaphoreがシグナル状態になったらレンダリング開始(WaitSemaphore)
    // レンダリングが終わったらimgRenderedSemaphoreにシグナル
    // imgRenderedSemaphoreがシグナル状態になったらプレゼン開始
    
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        
        auto waitRes = device->waitForFences(inFlight[currentFrame].get(), VK_TRUE, UINT64_MAX);
        if(waitRes != vk::Result::eSuccess){
            std::cerr << "can't wait for fence" << std::endl;
            return;
        }
        
        vk::ResultValue acquireImgResult = device->acquireNextImageKHR(swapchain.get(), 1'000'000'000, imageAvailable[currentFrame].get());
        if(acquireImgResult.result == vk::Result::eSuboptimalKHR || acquireImgResult.result == vk::Result::eErrorOutOfDateKHR){
            std::cerr << "recreate swapchain" << std::endl;
            recreateSwapchain();
            continue;
        }
        if(acquireImgResult.result != vk::Result::eSuccess){
            std::cerr << "can't get next image" << std::endl;
            return;
        }
        uint32_t imgIndex = acquireImgResult.value;

        if(imagesInFlight[imgIndex] != VK_NULL_HANDLE){
            waitRes = device->waitForFences(imagesInFlight[imgIndex], VK_TRUE, UINT64_MAX);
            if(waitRes != vk::Result::eSuccess){
                std::cerr << "can't wait for fence" << std::endl;
                return;
            }
        }

        imagesInFlight[imgIndex] = inFlight[currentFrame].get();

        device->resetFences(inFlight[currentFrame].get());

        updateDescriptorSet(imgIndex, *swapchainImageViews[imgIndex]);

        auto& cmdBuf = cmdBufs[currentFrame];
        cmdBuf->reset();

        vk::CommandBufferBeginInfo cmdBeginInfo{};
        cmdBuf->begin(cmdBeginInfo);

        vk::ImageSubresourceRange range{};
        range.aspectMask = vk::ImageAspectFlagBits::eColor;
        range.baseMipLevel = 0; range.levelCount  = 1;
        range.baseArrayLayer = 0; range.layerCount = 1;

        vk::ImageMemoryBarrier toGeneral{};
        toGeneral.oldLayout = vk::ImageLayout::eUndefined;
        toGeneral.newLayout = vk::ImageLayout::eGeneral;
        toGeneral.srcAccessMask = {};
        toGeneral.dstAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        toGeneral.image = swapchainImages[imgIndex];
        toGeneral.subresourceRange = range;

        cmdBuf->pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {}, nullptr, nullptr, toGeneral);

        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        cmdBuf->bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.get());
        cmdBuf->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipelineLayout.get(), 0, {descSets[imgIndex].get()}, {});
        cmdBuf->traceRaysKHR(
            raygenRegion,
            missRegion,
            hitRegion,
            {},
            width, height, 1
        );

        // trace 後：GENERAL → PRESENT
        vk::ImageMemoryBarrier toPresent{};
        toPresent.oldLayout = vk::ImageLayout::eGeneral;
        toPresent.newLayout = vk::ImageLayout::ePresentSrcKHR;
        toPresent.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        toPresent.dstAccessMask = {};
        toPresent.image = swapchainImages[imgIndex];
        toPresent.subresourceRange = range;

        cmdBuf->pipelineBarrier(
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {}, nullptr, nullptr, toPresent);

        cmdBuf->end();

        vk::CommandBuffer submitCmdBuf[1] = {cmdBuf.get()};
        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBufferCount(1);
        submitInfo.setPCommandBuffers(submitCmdBuf);

        vk::PipelineStageFlags renderwaitStages[] = {vk::PipelineStageFlagBits::eRayTracingShaderKHR};
        submitInfo.setWaitSemaphoreCount(1);
        submitInfo.setWaitSemaphores(imageAvailable[currentFrame].get());
        submitInfo.setPWaitDstStageMask(renderwaitStages);

        submitInfo.setSignalSemaphoreCount(1);
        submitInfo.setPSignalSemaphores(&renderFinished[imgIndex].get());
        
        queue.submit({submitInfo}, inFlight[currentFrame].get());

        vk::PresentInfoKHR presentInfo{};
        auto presentSwapchains = {swapchain.get()};

        presentInfo.setSwapchainCount(presentSwapchains.size());
        presentInfo.setPSwapchains(presentSwapchains.begin());
        presentInfo.setImageIndices(imgIndex);

        presentInfo.setWaitSemaphoreCount(1);
        presentInfo.setPWaitSemaphores(&renderFinished[imgIndex].get());

        auto res = queue.presentKHR(presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES;
        if(res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR){
            // recreateSwapchain();
            // continue;
        }else if(res != vk::Result::eSuccess){
            throw std::runtime_error("presentKHR failed");
        }
    }
    queue.waitIdle();
    glfwTerminate();
    return;
}