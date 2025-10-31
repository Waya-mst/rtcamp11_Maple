#include "../include/globals.hpp"
#include <iostream>

int SetupGLFW(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "vulkan programming", NULL, NULL);

    if(!glfwVulkanSupported()){
        std::cerr << "GLFW: Vulkan Not Supported" << std::endl;
        return 1;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return 0;
}