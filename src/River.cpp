#include "River.h"

#include <glm/glm.hpp>

#include <cmath>

namespace frozenriver {
namespace {
constexpr float pi = 3.14159265359f;

// A shared grid builder calculates normals from the actual triangles, including
// sloping drift ribbons and vertical walls. Winding faces up or toward the river.
template <typename Position>
SurfaceData grid(int columns, bool reverse, Position position) {
    SurfaceData data;
    const int stride = columns + 1;
    data.vertices.reserve((segments + 1) * stride * 6);
    data.indices.reserve(segments * columns * 6);
    for (int row = 0; row <= segments; ++row) {
        const float z = minimumZ + (maximumZ - minimumZ) * row / segments;
        for (int col = 0; col <= columns; ++col) {
            const glm::vec3 p = position(static_cast<float>(col) / columns, z);
            data.vertices.insert(data.vertices.end(), {p.x, p.y, p.z, 0, 0, 0});
        }
    }
    for (int row = 0; row < segments; ++row) {
        for (int col = 0; col < columns; ++col) {
            const unsigned int a = row * stride + col;
            const unsigned int b = a + 1;
            const unsigned int c = a + stride;
            const unsigned int d = c + 1;
            if (reverse) data.indices.insert(data.indices.end(), {a, d, c, a, b, d});
            else data.indices.insert(data.indices.end(), {a, c, d, a, d, b});
        }
    }
    const auto point = [&data](unsigned int i) {
        return glm::vec3(data.vertices[i * 6], data.vertices[i * 6 + 1], data.vertices[i * 6 + 2]);
    };
    for (std::size_t i = 0; i < data.indices.size(); i += 3) {
        const auto a = data.indices[i], b = data.indices[i + 1], c = data.indices[i + 2];
        const glm::vec3 n = glm::cross(point(b) - point(a), point(c) - point(a));
        for (auto vertex : {a, b, c}) {
            for (int axis = 0; axis < 3; ++axis) data.vertices[vertex * 6 + 3 + axis] += n[axis];
        }
    }
    for (std::size_t i = 0; i < data.vertices.size(); i += 6) {
        const glm::vec3 n = glm::normalize(glm::vec3(data.vertices[i + 3], data.vertices[i + 4], data.vertices[i + 5]));
        for (int axis = 0; axis < 3; ++axis) data.vertices[i + 3 + axis] = n[axis];
    }
    return data;
}
} // namespace

float riverCenter(float z) { return 1.75f + 1.30f * std::sin((z + 1.5f) * 0.16f); }
float riverHalfWidth(float z) { return 2.45f + 0.052f * (z + 12.0f); }

float snowHeight(float x, float z) {
    const auto mound = [x, z](float cx, float cz, float height, float radius) {
        const float dx = x - cx, dz = z - cz;
        return height * std::exp(-(dx * dx + dz * dz) / (radius * radius));
    };
    return 0.34f + 0.05f * std::sin(x * 0.50f) + 0.04f * std::cos(z * 0.42f)
        + mound(-8.0f, 7.0f, 1.65f, 3.0f) + mound(-4.3f, 9.0f, 0.95f, 2.3f)
        + mound(-9.5f, 1.0f, 0.65f, 2.8f) + mound(9.2f, 5.5f, 0.65f, 3.2f);
}

float iceHeight(float x, float z) {
    // Millimetre-scale relief: frozen solid, without animated water-like waves.
    return 0.055f + 0.004f * std::sin(x * 2.3f + z * 0.7f) * std::cos(z * 1.8f);
}

float driftWidth(float z, bool leftBank) {
    const float phase = leftBank ? 0.0f : 2.3f;
    return 0.46f + 0.16f * std::sin(z * 0.73f + phase)
        + 0.12f * std::sin(z * 1.41f + phase)
        + 0.16f * std::pow(0.5f + 0.5f * std::cos(z * 0.31f + phase), 3.0f);
}

SurfaceData createRiver() {
    return grid(32, false, [](float t, float z) {
        const float x = riverCenter(z) + (2.0f * t - 1.0f) * riverHalfWidth(z);
        return glm::vec3(x, iceHeight(x, z), z);
    });
}

SurfaceData createSnowBank(bool leftBank) {
    return grid(24, false, [leftBank](float t, float z) {
        const float edge = riverCenter(z) + (leftBank ? -1.0f : 1.0f) * riverHalfWidth(z);
        const float low = leftBank ? -outerEdge : edge;
        const float high = leftBank ? edge : outerEdge;
        const float x = low + (high - low) * t;
        return glm::vec3(x, snowHeight(x, z), z);
    });
}

SurfaceData createBankWall(bool leftBank) {
    return grid(1, !leftBank, [leftBank](float t, float z) {
        const float x = riverCenter(z) + (leftBank ? -1.0f : 1.0f) * riverHalfWidth(z);
        const float bottom = iceHeight(x, z) - 0.04f;
        return glm::vec3(x, (1.0f - t) * snowHeight(x, z) + t * bottom, z);
    });
}

SurfaceData createSnowDrift(bool leftBank) {
    return grid(12, !leftBank, [leftBank](float t, float z) {
        const float sign = leftBank ? -1.0f : 1.0f;
        const float edge = riverCenter(z) + sign * riverHalfWidth(z);
        const float x = edge - sign * driftWidth(z, leftBank) * t;
        // The root exactly meets the bank; the toe sits just above the ice.
        const float blend = t * t * (3.0f - 2.0f * t);
        const float y = (1.0f - blend) * snowHeight(edge, z)
            + blend * (iceHeight(x, z) + 0.006f)
            + 0.065f * std::sin(pi * t) * (0.7f + 0.3f * std::sin(z * 1.3f));
        return glm::vec3(x, y, z);
    });
}
} // namespace frozenriver
