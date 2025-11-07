#include "../include/globals.hpp"
#include "../include/vk_setup.hpp"
#include "../include/uniform.hpp"
#include "../include/descriptors.hpp"
#include "../include/loader.hpp"
#include "../include/geometry.hpp"
#include "../include/shaders.hpp"
#include "../include/render.hpp"
#include "../include/output.hpp"
#include <iostream>

int main(){
    auto exeDir = std::filesystem::current_path();
    SetupVulkan();
    createOutputBuffer();
    createUniformBuffer();
    loadResources(exeDir);
    createDescriptor(1);
    createBLAS();
    createTLAS();
    prepareShaders();
    createRayTracingPipeline();
    createShaderBindingTable();
    drawCall(exeDir);
    return 0;
}