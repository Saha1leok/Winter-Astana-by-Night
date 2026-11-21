#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace astana {

struct SurfaceData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

float riverCenter(float z);
float riverHalfWidth(float z);
float snowHeight(float x, float z);
SurfaceData createSnowBank(bool leftBank);
SurfaceData createRiver();
SurfaceData createBankWall(bool leftBank);
SurfaceData radialMesh(const std::vector<glm::vec2>& profile, int sides = 16);
std::vector<float> cubeVertices();

}
