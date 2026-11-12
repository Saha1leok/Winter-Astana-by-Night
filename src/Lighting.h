#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace lighting {
constexpr int maxLamps = 12; // Must match MAX_LAMPS in scene.frag.
struct Window {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
    bool occupied;
};
struct Lamp {
    glm::vec3 base;
    glm::vec3 bulb;
};
struct State {
    bool windows = true;
    bool lamps = true;
    void toggleAll() { const bool on = !(windows || lamps); windows = on; lamps = on; }
};

// Axis-aligned building, with the same world-space center/size as its mesh.
// Geometry is generated once; the seed makes occupied rooms reproducible.
std::vector<Window> buildingWindows(glm::vec3 center, glm::vec3 size, std::uint32_t seed);
void uploadLamps(unsigned int program, const std::vector<Lamp>& lamps, bool enabled);
} // namespace lighting
