#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

constexpr double M_PI = 3.141592653589793;

// ------------------------------------------------------------

glm::vec3 CubemapDirectionFromFaceXY(int face, int x, int y, int faceSize)
{
    // [-1,1] に正規化したテクスチャ座標
    float s = ( (x + 0.5f) / (float)faceSize ) * 2.0f - 1.0f;
    float t = ( (y + 0.5f) / (float)faceSize ) * 2.0f - 1.0f;
    t = -t;

    glm::vec3 dir;
    switch (face)
    {
    case 0: // +X
        dir = glm::vec3( 1.0f,     t,    -s);
        break;
    case 1: // -X
        dir = glm::vec3(-1.0f,     t,     s);
        break;
    case 2: // +Y
        dir = glm::vec3(   s,  1.0f,    t);
        break;
    case 3: // -Y
        dir = glm::vec3(   s, -1.0f,   -t);
        break;
    case 4: // +Z
        dir = glm::vec3(   s,     t,  1.0f);
        break;
    case 5: // -Z
        dir = glm::vec3(  -s,     t, -1.0f);
        break;
    }
    return glm::normalize(dir);
}

glm::vec4 SampleEquirect(const glm::vec3& dir, const float* src, int w, int h)
{
    // dir -> (u,v)
    float u = std::atan2(dir.z, dir.x) / (2.0f * float(M_PI)) + 0.5f;
    float v = std::acos(std::clamp(dir.y, -1.0f, 1.0f)) / float(M_PI);

    int px = std::clamp(int(u * w), 0, w - 1);
    int py = std::clamp(int(v * h), 0, h - 1);

    const float* p = src + (py * w + px) * 4;
    return glm::vec4(p[0], p[1], p[2], p[3]);
}

// ------------------------------------------------------------

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: gen_env_header <input.hdr> <output_basename>\n");
        std::fprintf(stderr, "example: gen_env_header env.hdr env_cubemap\n");
        return 1;
    }

    const char* inPath       = argv[1];
    std::string outBaseName  = argv[2];   // "env_cubemap" を想定
    std::string outHeader    = "./include/" + outBaseName + ".hpp";
    std::string outSource    = "./src/" + outBaseName + ".cpp";

    int envWidth, envHeight, envCh;
    float* pEnvData = stbi_loadf(inPath, &envWidth, &envHeight, &envCh, STBI_rgb_alpha);
    if (!pEnvData) {
        std::fprintf(stderr, "failed to load hdr: %s\n", inPath);
        return 1;
    }

    size_t envDataSize = 4 * envWidth * envHeight * sizeof(float);

    const int faceSize = 2048;   // 必要に応じて変える
    size_t   facePixels = (size_t)faceSize * (size_t)faceSize;
    size_t   faceBytes  = facePixels * 4 * sizeof(float);   // RGBA32F
    size_t   totalBytes = faceBytes * 6;

    std::vector<float> cube(6ull * facePixels * 4);

    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < faceSize; ++y) {
            for (int x = 0; x < faceSize; ++x) {
                glm::vec3 dir = CubemapDirectionFromFaceXY(face, x, y, faceSize);
                glm::vec4 c   = SampleEquirect(dir, pEnvData, envWidth, envHeight);
                size_t idx = ( (size_t)face * facePixels + y * faceSize + x ) * 4;
                cube[idx + 0] = c.r;
                cube[idx + 1] = c.g;
                cube[idx + 2] = c.b;
                cube[idx + 3] = c.a;
            }
        }
    }

    stbi_image_free(pEnvData);

    // ====== .hpp (宣言だけ) ======
    {
        std::FILE* fh = std::fopen(outHeader.c_str(), "w");
        if (!fh) {
            std::fprintf(stderr, "failed to open %s\n", outHeader.c_str());
            return 1;
        }

        std::fprintf(fh, "#pragma once\n");
        std::fprintf(fh, "#include <cstdint>\n\n");
        std::fprintf(fh, "extern const uint32_t kEnvFaceSize;\n");
        std::fprintf(fh, "extern const uint32_t kEnvNumFaces;\n");
        std::fprintf(fh, "extern const uint32_t kEnvFloatCount;\n");
        std::fprintf(fh, "extern const float   kEnvCubeData[];\n");

        std::fclose(fh);
    }

    // ====== .cpp (実体) ======
    {
        std::FILE* fc = std::fopen(outSource.c_str(), "w");
        if (!fc) {
            std::fprintf(stderr, "failed to open %s\n", outSource.c_str());
            return 1;
        }

        std::fprintf(fc, "#include \"%s\"\n\n", outHeader.c_str());
        std::fprintf(fc, "const uint32_t kEnvFaceSize   = %d;\n", faceSize);
        std::fprintf(fc, "const uint32_t kEnvNumFaces   = %d;\n", 6);
        std::fprintf(fc, "const uint32_t kEnvFloatCount = %zu;\n\n", (size_t)6ull * facePixels * 4);
        std::fprintf(fc, "alignas(16) const float kEnvCubeData[%zd] = {\n", (int)6ull * facePixels * 4);

        for (size_t i = 0; i < 6ull * facePixels * 4; ++i) {
            std::fprintf(fc, "  %.9ff,", cube[i]);
            if ((i + 1) % 4 == 0) std::fprintf(fc, " ");
            if ((i + 1) % 16 == 0) std::fprintf(fc, "\n");
        }
        std::fprintf(fc, "\n};\n");

        std::fclose(fc);
    }

    std::printf("generated %s and %s\n", outHeader.c_str(), outSource.c_str());
    return 0;
}
