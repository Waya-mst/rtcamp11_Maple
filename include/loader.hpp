#pragma once
#include <glm/glm.hpp>
#include <iostream>

void loadModel(std::string resourcePath);
void loadTexture(std::string resourcePath);
void loadMaterial();
void loadResources(std::filesystem::path exeDir);