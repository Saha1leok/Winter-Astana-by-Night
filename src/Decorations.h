#pragma once

#include <array>

namespace astana {

class Mesh;

void drawHolidayGarlands(const Mesh& cube, unsigned int program,
                         const std::array<float, 4>& blockLights,
                         bool enabled, double seconds);

}
