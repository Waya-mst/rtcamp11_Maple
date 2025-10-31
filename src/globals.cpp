#include "../include/globals.hpp"

const uint32_t width  = 1280;
const uint32_t height = 960;
#ifndef NDEBUG
bool enableValidationLayers = true;
#else
bool enableValidationLayers = false;
#endif

uint32_t alignUp(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

SceneUBO scene = {
    {
        glm::vec4(glm::normalize(glm::vec3(-0.5f, 1.0f, -0.3f)), 0.0f),
        glm::vec4(0.2f, 0.2f, 0.2f, 0.0f)
    },
    glm::vec4(-1.0f, 2.0f, 3.0f, 1.0f)
};

std::vector<Vertex> vertices;
std::vector<uint32_t> indices;

GLFWwindow* window;
std::vector<const char*> extensions;

vk::UniqueInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;

VkSurfaceKHR c_surface;
vk::UniqueSurfaceKHR surface;
vk::SurfaceCapabilitiesKHR surfaceCapabilities;

vk::UniqueDevice device;
vk::PhysicalDevice physicalDevice;

vk::UniqueSwapchainKHR swapchain;
vk::SurfaceFormatKHR swapchainFormat;
vk::PresentModeKHR swapchainPresentMode;
std::vector<vk::Image> swapchainImages;
std::vector<vk::UniqueImageView> swapchainImageViews;
std::vector<vk::UniqueFramebuffer> swapchainFramebufs;
vk::UniqueFence swapchainImgFence;

vk::Queue queue;
uint32_t queueFamily = (uint32_t)-1;

vk::UniqueCommandPool commandPool;
std::vector<vk::UniqueCommandBuffer> cmdBufs;

vk::UniquePipeline pipeline;
vk::UniquePipelineLayout pipelineLayout;

vk::UniqueShaderModule vertShader;
vk::UniqueShaderModule fragShader;

std::vector<vk::UniqueShaderModule> shaderModules;
std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

Buffer vertexBuffer;
Buffer indexBuffer;
Buffer sceneBuffer;
std::vector<Buffer> textureBuffers(1);

Buffer outputBuffer;
vk::UniqueImage outputImage;
vk::UniqueDeviceMemory outputMemory;
vk::UniqueImageView outputView;

vk::UniqueImage image;

vk::UniqueImage textureImage;
vk::UniqueDeviceMemory textureMemory;
vk::UniqueSampler sampler;
vk::UniqueImageView textureImageView;

vk::UniqueBuffer uniformBuffer;
vk::UniqueDeviceMemory uniformBufferMemory;
void* uniformData;
vk::UniqueDescriptorSetLayout descSetLayout;
vk::UniqueDescriptorPool descPool;
std::vector<vk::UniqueDescriptorSet> descSets;

AccelStruct bottomAccel{};
AccelStruct topAccel{};

Buffer sbt{};
vk::StridedDeviceAddressRegionKHR raygenRegion{};
vk::StridedDeviceAddressRegionKHR missRegion{};
vk::StridedDeviceAddressRegionKHR hitRegion{};

tinygltf::Model model;
tinygltf::TinyGLTF loader;