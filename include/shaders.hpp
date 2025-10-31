#pragma once
#include "globals.hpp"
#include <iostream>

void prepareShaders();
void addShader(uint32_t shaderIndex, const std::string& filename, vk::ShaderStageFlagBits stage);
void createRayTracingPipeline();
void createShaderBindingTable();