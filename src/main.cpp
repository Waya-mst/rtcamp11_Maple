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
    SetupVulkan();
    createOutputBuffer();
    createUniformBuffer();
    createDescriptor(1);
    loadModel();
    loadMaterial();
    loadTexture();
    createBLAS();
    createTLAS();
    prepareShaders();
    createRayTracingPipeline();
    createShaderBindingTable();
    drawCall();
    return 0;
}