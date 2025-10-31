#include "../include/globals.hpp"
#include "../include/window.hpp"
#include "../include/vk_setup.hpp"
#include "../include/uniform.hpp"
#include "../include/descriptors.hpp"
#include "../include/loader.hpp"
#include "../include/geometry.hpp"
#include "../include/shaders.hpp"
#include "../include/render.hpp"
#include <iostream>

int main(){
    SetupGLFW();
    SetupVulkan(extensions);
    recreateSwapchain();
    createUniformBuffer();
    createDescriptor(swapchainImages.size());
    loadModel();
    loadTexture();
    createBLAS();
    createTLAS();
    prepareShaders();
    createRayTracingPipeline();
    createShaderBindingTable();
    drawCall();
    return 0;
}