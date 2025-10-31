#pragma once
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "accel.hpp"
#include <GLFW/glfw3.h>

#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <nlohmann/json.hpp>
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

inline constexpr uint32_t MAX_FRAMES = 2;
extern const uint32_t width;
extern const uint32_t height;
extern bool enableValidationLayers;

uint32_t alignUp(uint32_t size, uint32_t alignment);

struct Mat4x4{ float v[4][4]; };
struct Vertex{ alignas(16) glm::vec3 pos; alignas(16) glm::vec3 normal; alignas(16) glm::vec2 texCoord; };
struct Light { alignas(16) glm::vec4 dir; alignas(16) glm::vec4 color; };
struct SceneUBO { Light sun; alignas(16) glm::vec4 camPos; };

extern SceneUBO scene;
extern std::vector<Vertex> vertices;
extern std::vector<uint32_t> indices;

extern GLFWwindow* window;
extern std::vector<const char*> extensions;

extern vk::UniqueImage image;

extern VkSurfaceKHR c_surface;
extern vk::UniqueSurfaceKHR surface;
extern vk::SurfaceCapabilitiesKHR surfaceCapabilities;

extern vk::UniqueSwapchainKHR swapchain;
extern vk::SurfaceFormatKHR swapchainFormat;
extern vk::PresentModeKHR swapchainPresentMode;
extern std::vector<vk::Image> swapchainImages;
extern std::vector<vk::UniqueImageView> swapchainImageViews;
extern std::vector<vk::UniqueFramebuffer> swapchainFramebufs;
extern vk::UniqueFence swapchainImgFence;

extern vk::UniqueInstance instance;
extern VkDebugUtilsMessengerEXT debugMessenger;
extern vk::UniqueDevice device;
extern vk::PhysicalDevice physicalDevice;
extern vk::Queue queue;
extern uint32_t queueFamily;

extern vk::UniqueCommandPool commandPool;
extern std::vector<vk::UniqueCommandBuffer> cmdBufs;

extern vk::UniquePipeline pipeline;
extern vk::UniquePipelineLayout pipelineLayout;

extern vk::UniqueShaderModule vertShader;
extern vk::UniqueShaderModule fragShader;

extern std::vector<vk::UniqueShaderModule> shaderModules;
extern std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
extern std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

struct Buffer;
struct AccelStruct;

extern Buffer vertexBuffer;
extern Buffer indexBuffer;
extern Buffer sceneBuffer;
extern std::vector<Buffer> textureBuffers;
extern vk::UniqueImage textureImage;
extern vk::UniqueDeviceMemory textureMemory;
extern vk::UniqueSampler sampler;
extern vk::UniqueImageView textureImageView;

extern vk::UniqueBuffer uniformBuffer;
extern vk::UniqueDeviceMemory uniformBufferMemory;
extern void* uniformData;
extern vk::UniqueDescriptorSetLayout descSetLayout;
extern vk::UniqueDescriptorPool descPool;
extern std::vector<vk::UniqueDescriptorSet> descSets;

extern AccelStruct bottomAccel;
extern AccelStruct topAccel;

extern Buffer sbt;
extern vk::StridedDeviceAddressRegionKHR raygenRegion;
extern vk::StridedDeviceAddressRegionKHR missRegion;
extern vk::StridedDeviceAddressRegionKHR hitRegion;

extern tinygltf::Model model;
extern tinygltf::TinyGLTF loader;
