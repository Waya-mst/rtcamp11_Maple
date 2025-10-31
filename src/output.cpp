#include "../include/globals.hpp"

void createOutputBuffer(){
    vk::DeviceSize size = width * height * 4;
    outputBuffer.init(
        physicalDevice, *device, size,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    

    vk::ImageCreateInfo ci{};
    ci.setImageType(vk::ImageType::e2D);
    ci.setExtent({uint32_t(width), uint32_t(height), 1});
    ci.setMipLevels(1); ci.setArrayLayers(1);
    ci.setFormat(vk::Format::eR8G8B8A8Unorm);
    ci.setTiling(vk::ImageTiling::eOptimal);
    ci.setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);
    ci.setSamples(vk::SampleCountFlagBits::e1);
    ci.setSharingMode(vk::SharingMode::eExclusive);

    outputImage = device->createImageUnique(ci);

    auto req = device->getImageMemoryRequirements(outputImage.get());
    uint32_t memIndex = 0;
    for (uint32_t i = 0; i < physicalDevice.getMemoryProperties().memoryTypeCount; ++i) {
        if ((req.memoryTypeBits & (1u << i)) &&
            (physicalDevice.getMemoryProperties().memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
            memIndex = i; break;
        }
    }
    outputMemory = device->allocateMemoryUnique({req.size, memIndex});
    device->bindImageMemory(outputImage.get(), outputMemory.get(), 0);

    vk::ImageViewCreateInfo vci{};
    vci.image = outputImage.get();
    vci.viewType = vk::ImageViewType::e2D;
    vci.format = ci.format;
    vci.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    outputView = device->createImageViewUnique(vci);
}
