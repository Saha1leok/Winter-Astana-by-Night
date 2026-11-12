#include "Lighting.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace lighting {
std::vector<Window> buildingWindows(glm::vec3 center, glm::vec3 size, std::uint32_t seed) {
    std::vector<Window> windows;
    const int rows = std::max(1, static_cast<int>(size.y / 0.65f));
    // A small integer PRNG avoids implementation-dependent distribution results.
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int face = 0; face < 4; ++face) {
        const bool frontBack = face < 2;
        const float sign = face % 2 == 0 ? 1.0f : -1.0f;
        const float width = frontBack ? size.x : size.z;
        const int columns = std::max(1, static_cast<int>(width / 0.55f));
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                const float across = ((column + 0.5f) / columns - 0.5f) * width;
                const float y = ((row + 0.5f) / rows - 0.5f) * size.y;
                const glm::vec3 offset = frontBack
                    ? glm::vec3(across, y, sign * (size.z * 0.5f + 0.014f))
                    : glm::vec3(sign * (size.x * 0.5f + 0.014f), y, across);
                const bool occupied = next() % 100u < 67u;
                const float warmth = static_cast<float>(next() % 100u) / 100.0f;
                windows.push_back({center + offset,
                    frontBack ? glm::vec3(0.25f, 0.32f, 0.018f) : glm::vec3(0.018f, 0.32f, 0.25f),
                    glm::vec3(1.0f, 0.56f + warmth * 0.20f, 0.18f + warmth * 0.12f), occupied});
            }
        }
    }
    return windows;
}

void uploadLamps(unsigned int program, const std::vector<Lamp>& lamps, bool enabled) {
    if (lamps.size() > maxLamps) throw std::runtime_error("Too many lamps for scene.frag");
    glUniform1i(glGetUniformLocation(program, "uLampCount"), enabled ? static_cast<int>(lamps.size()) : 0);
    if (!enabled) return;
    for (std::size_t i = 0; i < lamps.size(); ++i) {
        const std::string name = "uLampPositions[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(lamps[i].bulb));
    }
}
} // namespace lighting
