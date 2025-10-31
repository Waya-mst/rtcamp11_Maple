#include "../include/globals.hpp"
#include "../include/window.hpp"
#include <iostream>


VKAPI_ATTR VkBool32 VKAPI_CALL debug_message(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* pUserData
);

void SetupVulkan(std::vector<const char*>& extensions){
    VkResult err;

	// create instance
	vk::detail::DynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	std::vector layers{ "VK_LAYER_KHRONOS_validation" };

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("vulkan programming");
    appInfo.setApiVersion(VK_API_VERSION_1_3);

	vk::InstanceCreateInfo instanceInfo = {};
    instanceInfo.setPApplicationInfo(&appInfo);
	instanceInfo.setPEnabledExtensionNames(extensions);
	instanceInfo.setEnabledExtensionCount(extensions.size());
	instanceInfo.setPEnabledLayerNames(layers);
	instanceInfo.setEnabledLayerCount(layers.size());
#if defined(__APPLE__)
	instanceInfo.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
#endif
	
	instance = vk::createInstanceUnique(instanceInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    // create surface
    VkResult result = glfwCreateWindowSurface(instance.get(), window, nullptr, &c_surface);
    if(result != VK_SUCCESS){
        const char* err;
        glfwGetError(&err);
        std::cout << err << std::endl;
        glfwTerminate();
        return;
    }
    surface = vk::UniqueSurfaceKHR{c_surface, instance.get()};

    // create debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = debug_message;
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance.get(), "vkCreateDebugUtilsMessengerEXT");
    result = vkCreateDebugUtilsMessengerEXT(instance.get(), &debugMessengerInfo, nullptr, &debugMessenger);

    // select Device
    std::vector devices = instance->enumeratePhysicalDevices();
    int deviceIndex = 0;

    // Physical device Type
    vk::PhysicalDeviceType wantedType = 
#if defined(__APPLE__)
    vk::PhysicalDeviceType::eIntegratedGpu;
#elif defined(_WIN32)
    vk::PhysicalDeviceType::eDiscreteGpu;
#else
    vk::PhysicalDeviceType::eDiscreteGpu;
#endif

    for(size_t i = 0; i < devices.size(); i++){
        vk::PhysicalDeviceProperties props = devices[i].getProperties();
        if(props.deviceType == wantedType){
            std::vector<vk::QueueFamilyProperties> queueProps = devices[i].getQueueFamilyProperties();
            bool existsGraphicsQueue = false;

            for(size_t j = 0; j < queueProps.size(); j++){
                if(queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics && devices[i].getSurfaceSupportKHR(j, surface.get())){
                    existsGraphicsQueue = true;
                    queueFamily = j;
                    break;
                }
            }

            std::vector<vk::ExtensionProperties> extProps = devices[i].enumerateDeviceExtensionProperties();
            bool supportSwapchainExtension = false;

            for(size_t j = 0; j < extProps.size(); j++){
                if(std::string_view(extProps[j].extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME){
                    supportSwapchainExtension = true;
                    break;
                }
            }

            bool supportsSurface =
                !devices[i].getSurfaceFormatsKHR(surface.get()).empty() ||
                !devices[i].getSurfacePresentModesKHR(surface.get()).empty();

            if(existsGraphicsQueue && supportSwapchainExtension && supportsSurface){
                physicalDevice = devices[i];
                break;
            }
        }
    }

    // create Logical Device
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        //VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    };
    vk::PhysicalDeviceFeatures2 feats2{};
    vk::PhysicalDeviceVulkan12Features v12{};
    v12.bufferDeviceAddress = VK_TRUE;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeat{};
    asFeat.accelerationStructure = VK_TRUE;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtpFeat{};
    rtpFeat.rayTracingPipeline = VK_TRUE;

    v12.pNext   = &asFeat;
    asFeat.pNext= &rtpFeat;
    feats2.pNext= &v12;

    const float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.setQueueFamilyIndex(queueFamily);
	queueCreateInfo.setQueueCount(1);
	queueCreateInfo.setPQueuePriorities(&queuePriority);

    vk::DeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.setQueueCreateInfos(queueCreateInfo);
    deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);
    deviceCreateInfo.setPNext(&feats2);
    device = physicalDevice.createDeviceUnique(deviceCreateInfo);

    queue = device->getQueue(queueFamily, 0);

    // initialize swapchain formats
    surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());
    
    swapchainFormat = surfaceFormats[0];
    swapchainPresentMode = surfacePresentModes[0];

    // create Command Pool
    vk::CommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);
    commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
}

void recreateSwapchain(){
    swapchainFramebufs.clear();
    swapchainImageViews.clear();
    swapchainImages.clear();
    swapchain.reset();

    surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.setSurface(surface.get());
    swapchainCreateInfo.surface = surface.get();
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    swapchainCreateInfo.imageFormat = swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

    swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

    swapchainImageViews.resize(swapchainImages.size());

    for(size_t i = 0; i < swapchainImages.size(); i++){
        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = swapchainImages[i];
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imgViewCreateInfo.format = swapchainFormat.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        swapchainImageViews[i] = device->createImageViewUnique(imgViewCreateInfo);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_message(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}