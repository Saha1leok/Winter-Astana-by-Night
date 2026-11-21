#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace astana {

class Mesh;

struct CityBuilding {
    int blockId;
    glm::vec3 center;
    glm::vec3 size;
    glm::vec3 facade;
    int style;
};

struct StreetlightAnchor {
    int blockId;
    glm::vec3 position;
};

struct ParkFir {
    float x;
    float z;
    float scale;
    int blockId;
};

const std::vector<CityBuilding>& cityBuildings();
const std::vector<StreetlightAnchor>& cityStreetlightAnchors();
const std::array<ParkFir, 12>& parkFirs();

void cityBeam(const Mesh& cube, unsigned int program, const glm::vec3& from,
              const glm::vec3& to, float width, const glm::vec3& color);
void drawCityLayout(const Mesh& cube, const Mesh& khanRoof, const Mesh& firCrown,
                    const Mesh& firSnowCap, const Mesh& bayterekGlobe,
                    const Mesh& akOrdaDome, unsigned int program,
                    float snowCoverage, const std::array<float, 4>& blockLights);

}
