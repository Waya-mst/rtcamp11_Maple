#include "../include/globals.hpp"
#include <iostream>


VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_message(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
);

void SetupVulkan(){
	// create instance
	vk::detail::DynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    auto availableExts = vk::enumerateInstanceExtensionProperties();
    std::vector<const char*> enabledExtensions;

    for (auto we : extensions) {
        bool found = false;
        for (auto const& e : availableExts) {
            if (strcmp(e.extensionName, we) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabledExtensions.push_back(we);
        } else {
            std::cout << "extension not present: " << we << "\n";
        }
    }

	std::vector wantedlayers{ "VK_LAYER_KHRONOS_validation" };

    std::vector<vk::LayerProperties> available = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> enabledLayers;

    for (auto wl : wantedlayers) {
        bool found = false;
        for (auto const& a : available) {
            if (strcmp(a.layerName, wl) == 0) {
                found = true;
                break;
            }
        }
        if (found) {
            enabledLayers.push_back(wl);
        } else {
            std::cout << "can't find required layer extension" << std::endl;
        }
    }

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("vulkan programming");
    appInfo.setApiVersion(VK_API_VERSION_1_3);

	vk::InstanceCreateInfo instanceInfo = {};
    instanceInfo.setPApplicationInfo(&appInfo);
	// instanceInfo.setPEnabledExtensionNames(extensions);
	// instanceInfo.setEnabledExtensionCount(extensions.size());
	instanceInfo.setPEnabledLayerNames(enabledLayers);
	instanceInfo.setEnabledLayerCount(enabledLayers.size());
#if defined(__APPLE__)
	instanceInfo.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
#endif
	
	instance = vk::createInstanceUnique(instanceInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

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
                if(queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics){
                    existsGraphicsQueue = true;
                    queueFamily = j;
                    break;
                }
            }

            std::vector<vk::ExtensionProperties> extProps = devices[i].enumerateDeviceExtensionProperties();

            if(existsGraphicsQueue){
                physicalDevice = devices[i];
                break;
            }
        }
    }

    // create Logical Device
    std::vector<const char*> deviceExtensions = {
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

    v12.runtimeDescriptorArray = VK_TRUE;
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
    // create Command Pool
    vk::CommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);
    commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);

    VkPhysicalDeviceProperties physProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &physProps);
    std::cout << "PhysicalDevice: " << physProps.deviceName << std::endl;
    printf("Device API %u.%u.%u\n",
    VK_VERSION_MAJOR(physProps.apiVersion),
    VK_VERSION_MINOR(physProps.apiVersion),
    VK_VERSION_PATCH(physProps.apiVersion));
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_message(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cout << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}