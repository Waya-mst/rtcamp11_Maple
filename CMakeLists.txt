set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.20)
project(maple)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# FindPackage
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 REQUIRED)

find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
set(SLANG_SPV_PROFILE spirv_1_5 CACHE STRING "SPIR-V profile for slangc")

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SHADER_DIR ${SRC_DIR}/shader)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(APP_SOURCES
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/accel.cpp
  ${SRC_DIR}/buffer.cpp
  ${SRC_DIR}/descriptors.cpp
  ${SRC_DIR}/geometry.cpp
  ${SRC_DIR}/globals.cpp
  ${SRC_DIR}/loader.cpp
  ${SRC_DIR}/render.cpp
  ${SRC_DIR}/shaders.cpp
  ${SRC_DIR}/uniform.cpp
  ${SRC_DIR}/vk_setup.cpp
  ${SRC_DIR}/impl_tinygltf.cpp
  ${SRC_DIR}/impl_stb_image.cpp
  ${SRC_DIR}/impl_stb_image_write.cpp
  ${SRC_DIR}/vulkan_dispatch.cpp
  ${SRC_DIR}/output.cpp
)


# .slang -> raygen.spv
add_custom_command(
  OUTPUT  ${SHADER_OUT_DIR}/raygen.spv
  COMMAND ${SLANGC_EXECUTABLE}
          ${SHADER_DIR}/raygen.slang
          -target spirv
          -profile ${SLANG_SPV_PROFILE}
          -I ${SHADER_DIR}
          -entry raygenMain
          -stage raygeneration
          -o ${SHADER_OUT_DIR}/raygen.spv
  DEPENDS ${SHADER_DIR}/raygen.slang
  VERBATIM
)

# .slang -> miss_main.spv
add_custom_command(
  OUTPUT  ${SHADER_OUT_DIR}/miss_main.spv
  COMMAND ${SLANGC_EXECUTABLE}
          ${SHADER_DIR}/miss_main.slang
          -target spirv
          -profile ${SLANG_SPV_PROFILE}
          -I ${SHADER_DIR}
          -entry missMain
          -stage miss
          -o ${SHADER_OUT_DIR}/miss_main.spv
  DEPENDS ${SHADER_DIR}/miss_main.slang
  VERBATIM
)

# .slang -> miss_shadow.spv
add_custom_command(
  OUTPUT  ${SHADER_OUT_DIR}/miss_shadow.spv
  COMMAND ${SLANGC_EXECUTABLE}
          ${SHADER_DIR}/miss_shadow.slang
          -target spirv
          -profile ${SLANG_SPV_PROFILE}
          -I ${SHADER_DIR}
          -entry missShadow
          -stage miss
          -o ${SHADER_OUT_DIR}/miss_shadow.spv
  DEPENDS ${SHADER_DIR}/miss_shadow.slang
  VERBATIM
)

# .slang -> closesthit.spv
add_custom_command(
  OUTPUT  ${SHADER_OUT_DIR}/closesthit.spv
  COMMAND ${SLANGC_EXECUTABLE}
          ${SHADER_DIR}/closesthit.slang
          -target spirv
          -profile ${SLANG_SPV_PROFILE}
          -I ${SHADER_DIR}
          -entry closestHitMain
          -stage closesthit
          -o ${SHADER_OUT_DIR}/closesthit.spv
  DEPENDS ${SHADER_DIR}/closesthit.slang
  VERBATIM
)

# .slang -> anyhit.spv
add_custom_command(
  OUTPUT  ${SHADER_OUT_DIR}/anyhit.spv
  COMMAND ${SLANGC_EXECUTABLE}
          ${SHADER_DIR}/anyhit.slang
          -target spirv
          -profile ${SLANG_SPV_PROFILE}
          -I ${SHADER_DIR}
          -entry anyhitShadow
          -stage anyhit
          -o ${SHADER_OUT_DIR}/anyhit.spv
  DEPENDS ${SHADER_DIR}/anyhit.slang
  VERBATIM
)

# 生成物をまとめるターゲット
add_custom_target(shaders ALL
  DEPENDS
    ${SHADER_OUT_DIR}/raygen.spv
    ${SHADER_OUT_DIR}/miss_main.spv
    ${SHADER_OUT_DIR}/miss_shadow.spv
    ${SHADER_OUT_DIR}/closesthit.spv
    ${SHADER_OUT_DIR}/anyhit.spv
)

add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
  ${CMAKE_SOURCE_DIR}/libs
  ${CMAKE_SOURCE_DIR}/libs/nlohmann
)

# 実行ファイルがシェーダに依存
add_dependencies(${PROJECT_NAME} shaders)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${SRC_DIR}
)

# 実行用のカスタムターゲット
add_custom_target(run
  COMMAND $<TARGET_FILE:${PROJECT_NAME}>
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}  # 実行時のカレントをプロジェクトルートに
  USES_TERMINAL
  DEPENDS ${PROJECT_NAME}                # 先にビルドしてから実行
)

# 既に作ってある run ターゲットからも依存（ビルド→シェーダ生成→実行）
add_dependencies(run shaders)

target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options ( ${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /utf-8>)
target_compile_definitions(maple PRIVATE
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

if(APPLE)
    link_directories(($ENV{VULKAN_SDK}/lib))
    include_directories($ENV{VULKAN_SDK}/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VK_USE_PLATFORM_MACOS_MVK
    )
endif()

target_link_libraries( ${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glm::glm
    glfw
)
