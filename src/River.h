#pragma once

#include <vector>

// Member 4: all river/bank geometry uses the same shoreline and coordinate space.
namespace frozenriver {
constexpr float minimumZ = -12.0f;
constexpr float maximumZ = 12.0f;
constexpr float outerEdge = 13.5f;
constexpr int segments = 128;

struct SurfaceData {
    // Interleaved position.xyz, normal.xyz, matching the scene VAO layout.
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

float riverCenter(float z);
float riverHalfWidth(float z);
float snowHeight(float x, float z);
float iceHeight(float x, float z);
float driftWidth(float z, bool leftBank);
SurfaceData createRiver();
SurfaceData createSnowBank(bool leftBank);
SurfaceData createBankWall(bool leftBank);
SurfaceData createSnowDrift(bool leftBank);
} // namespace frozenriver
