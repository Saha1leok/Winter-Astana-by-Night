#include "Terrain.h"

#include <algorithm>
#include <cmath>

namespace astana {
namespace {

void addSurfaceVertex(std::vector<float>& vertices, float x, float y, float z) {
    vertices.insert(vertices.end(), {x, y, z, 0.0f, 1.0f, 0.0f});
}

void addSnowVertex(std::vector<float>& vertices, float x, float z) {
    constexpr float sampleStep = 0.08f;
    const float height = snowHeight(x, z);
    const float slopeX = (snowHeight(x + sampleStep, z) - snowHeight(x - sampleStep, z)) / (2.0f * sampleStep);
    const float slopeZ = (snowHeight(x, z + sampleStep) - snowHeight(x, z - sampleStep)) / (2.0f * sampleStep);
    const glm::vec3 terrainNormal = glm::normalize(glm::vec3(-slopeX, 1.0f, -slopeZ));
    const glm::vec3 normal = glm::normalize(glm::mix(terrainNormal, glm::vec3(0, 1, 0), 0.72f));
    vertices.insert(vertices.end(), {x, height, z, normal.x, normal.y, normal.z});
}

}

float riverCenter(float z) {
    return 3.4f + 1.75f * std::sin((z - 1.0f) * 0.15f) + 0.045f * z;
}

float riverHalfWidth(float z) {
    const float foreground = 0.5f + 0.5f * std::tanh((z - 1.0f) * 0.20f);
    return 1.72f + 2.45f * foreground + 0.10f * std::cos((z + 2.0f) * 0.22f);
}

float snowHeight(float x, float z) {
    const float base = 0.34f + 0.05f * std::sin(x * 0.50f) + 0.04f * std::cos(z * 0.42f);
    const auto mound = [x, z](float centerX, float centerZ, float height, float radius) {
        const float dx = x - centerX;
        const float dz = z - centerZ;
        return height * std::exp(-(dx * dx + dz * dz) / (radius * radius));
    };
    return base + mound(-8.0f, 7.0f, 0.72f, 3.0f)
        + mound(-4.3f, 9.0f, 0.46f, 2.3f)
        + mound(-9.5f, 1.0f, 0.42f, 2.8f)
        + mound(9.2f, 5.5f, 0.42f, 3.2f);
}

SurfaceData createSnowBank(bool leftBank) {
    constexpr int segments = 96;
    constexpr int columns = 24;
    constexpr float minimumZ = -16.0f;
    constexpr float maximumZ = 19.0f;
    constexpr float outerEdge = 19.0f;
    SurfaceData surface;
    surface.vertices.reserve((segments + 1) * (columns + 1) * 6);
    surface.indices.reserve(segments * columns * 6);

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float z = minimumZ + (maximumZ - minimumZ) * t;
        const float center = riverCenter(z);
        const float halfWidth = riverHalfWidth(z);
        const float lowX = leftBank ? -outerEdge : center + halfWidth;
        const float highX = leftBank ? center - halfWidth : outerEdge;
        for (int column = 0; column <= columns; ++column) {
            const float across = static_cast<float>(column) / static_cast<float>(columns);
            addSnowVertex(surface.vertices, lowX + (highX - lowX) * across, z);
        }
    }

    for (int i = 0; i < segments; ++i) {
        for (int column = 0; column < columns; ++column) {
            const unsigned int a = static_cast<unsigned int>(i * (columns + 1) + column);
            const unsigned int b = a + 1;
            const unsigned int c = a + static_cast<unsigned int>(columns + 1);
            const unsigned int d = c + 1;
            surface.indices.insert(surface.indices.end(), {a, c, d, a, d, b});
        }
    }
    return surface;
}

SurfaceData createRiver() {
    constexpr int segments = 48;
    constexpr float minimumZ = -16.0f;
    constexpr float maximumZ = 19.0f;
    SurfaceData surface;
    surface.vertices.reserve((segments + 1) * 12);
    surface.indices.reserve(segments * 6);

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float z = minimumZ + (maximumZ - minimumZ) * t;
        const float center = riverCenter(z);
        const float halfWidth = riverHalfWidth(z) + 0.18f;
        addSurfaceVertex(surface.vertices, center - halfWidth, 0.055f, z);
        addSurfaceVertex(surface.vertices, center + halfWidth, 0.055f, z);
    }
    for (int i = 0; i < segments; ++i) {
        const unsigned int a = static_cast<unsigned int>(i * 2);
        surface.indices.insert(surface.indices.end(), {a, a + 2, a + 3, a, a + 3, a + 1});
    }
    return surface;
}

SurfaceData createBankWall(bool leftBank) {
    constexpr int segments = 48;
    constexpr float minimumZ = -16.0f;
    constexpr float maximumZ = 19.0f;
    SurfaceData surface;
    surface.vertices.reserve((segments + 1) * 12);
    surface.indices.reserve(segments * 12);

    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float z = minimumZ + (maximumZ - minimumZ) * t;
        const float edgeX = riverCenter(z) + (leftBank ? -riverHalfWidth(z) : riverHalfWidth(z));
        addSurfaceVertex(surface.vertices, edgeX, snowHeight(edgeX, z) - 0.02f, z);
        addSurfaceVertex(surface.vertices, edgeX, 0.045f, z);
    }
    for (int i = 0; i < segments; ++i) {
        const unsigned int a = static_cast<unsigned int>(i * 2);
        const unsigned int b = a + 1;
        const unsigned int c = a + 2;
        const unsigned int d = a + 3;
        surface.indices.insert(surface.indices.end(), {a, c, d, a, d, b, a, d, c, a, b, d});
    }
    return surface;
}

SurfaceData radialMesh(const std::vector<glm::vec2>& profile, int sides) {
    SurfaceData result;
    constexpr float twoPi = 6.28318530718f;
    for (std::size_t ring = 0; ring < profile.size(); ++ring) {
        const std::size_t previous = ring == 0 ? ring : ring - 1;
        const std::size_t next = std::min(ring + 1, profile.size() - 1);
        const glm::vec2 profileNormal = glm::normalize(glm::vec2(
            profile[next].x - profile[previous].x,
            -(profile[next].y - profile[previous].y)));
        for (int side = 0; side <= sides; ++side) {
            const float angle = twoPi * static_cast<float>(side) / static_cast<float>(sides);
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            const float radius = profile[ring].y;
            result.vertices.insert(result.vertices.end(), {
                radius * cosine, profile[ring].x, radius * sine,
                profileNormal.x * cosine, profileNormal.y, profileNormal.x * sine
            });
        }
    }
    for (std::size_t ring = 0; ring + 1 < profile.size(); ++ring) {
        for (int side = 0; side < sides; ++side) {
            const unsigned int a = static_cast<unsigned int>(ring * (sides + 1) + side);
            const unsigned int b = a + static_cast<unsigned int>(sides + 1);
            result.indices.insert(result.indices.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }
    return result;
}

std::vector<float> cubeVertices() {
    return {
        -0.5f,-0.5f, 0.5f, 0,0,1,  0.5f,-0.5f, 0.5f, 0,0,1,  0.5f,0.5f,0.5f, 0,0,1, -0.5f,0.5f,0.5f, 0,0,1,
         0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f,0.5f,-0.5f, 0,0,-1, 0.5f,0.5f,-0.5f, 0,0,-1,
        -0.5f,-0.5f,-0.5f, -1,0,0, -0.5f,-0.5f,0.5f, -1,0,0, -0.5f,0.5f,0.5f, -1,0,0, -0.5f,0.5f,-0.5f, -1,0,0,
         0.5f,-0.5f,0.5f, 1,0,0, 0.5f,-0.5f,-0.5f, 1,0,0, 0.5f,0.5f,-0.5f, 1,0,0, 0.5f,0.5f,0.5f, 1,0,0,
        -0.5f,0.5f,0.5f, 0,1,0, 0.5f,0.5f,0.5f, 0,1,0, 0.5f,0.5f,-0.5f, 0,1,0, -0.5f,0.5f,-0.5f, 0,1,0,
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0.5f,-0.5f,-0.5f, 0,-1,0, 0.5f,-0.5f,0.5f, 0,-1,0, -0.5f,-0.5f,0.5f, 0,-1,0
    };
}

}
