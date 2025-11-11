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
    std::cout << "metallic: " << model.materials[0].pbrMetallicRoughness.metallicFactor << std::endl;
    std::cout << "roughness: " << model.materials[0].pbrMetallicRoughness.roughnessFactor << std::endl;
    std::cout << "emissive: " << model.materials[0].emissiveFactor[0] << ", " << model.materials[0].emissiveFactor[1] << ", " << model.materials[0].emissiveFactor[2] << std::endl;
    drawCall(exeDir);
    return 0;
}