#pragma once
#include "globals.hpp"

void createDescriptor(size_t countSets);
void updateDescriptorSet(uint32_t setIndex, vk::ImageView imageView);