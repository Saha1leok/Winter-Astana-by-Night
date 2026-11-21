#include "Decorations.h"

#include "CityScene.h"
#include "Mesh.h"
#include "RenderUtils.h"
#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace astana {

struct GarlandPart {
    glm::vec3 from;
    glm::vec3 to;
    int blockId;
    int bulbColor;
    float phase;
    bool isBulb;
    float size = 1.0f;
};

const std::vector<GarlandPart>& holidayGarlands() {
    static const std::vector<GarlandPart> parts = [] {
        std::vector<GarlandPart> result;
        result.reserve(1400);
        int colorNumber = 0;
        auto addString = [&](const glm::vec3& first, const glm::vec3& last,
                             int blockId, int bulbs, float sag) {
            glm::vec3 previous = first;
            for (int i = 1; i <= bulbs + 1; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(bulbs + 1);
                glm::vec3 point = glm::mix(first, last, t);
                point.y -= 4.0f * sag * t * (1.0f - t);
                result.push_back({previous, point, blockId, 0, 0.0f, false});
                if (i <= bulbs) {
                    result.push_back({point, point, blockId, colorNumber % 5,
                                      static_cast<float>(colorNumber) * 0.87f, true});
                    ++colorNumber;
                }
                previous = point;
            }
        };

        // Draped strings on the river promenade.
        for (int i = 0; i < 10; ++i) {
            const float z0 = -10.0f + 2.0f * i;
            const float z1 = z0 + 2.0f;
            const float x0 = riverCenter(z0) - riverHalfWidth(z0) - 0.58f;
            const float x1 = riverCenter(z1) - riverHalfWidth(z1) - 0.58f;
            addString({x0, snowHeight(x0,z0)+1.61f, z0},
                      {x1, snowHeight(x1,z1)+1.61f, z1},
                      z0 < -0.1f ? 1 : 2, 7, 0.20f);
        }

        // Two strings along the bridge railings.
        const float bridgeX = riverCenter(4.6f);
        for (float side : {-0.46f, 0.46f}) {
            for (int i = -1; i < 1; ++i) {
                addString({bridgeX + static_cast<float>(i)*1.55f, 1.52f, 4.6f+side},
                          {bridgeX + static_cast<float>(i+1)*1.55f, 1.52f, 4.6f+side},
                          4, 6, 0.17f);
            }
        }

        // Roofline bulbs on the taller city blocks.
        for (const auto& building : cityBuildings()) {
            if (building.size.y < 4.0f) continue;
            const float y = snowHeight(building.center.x, building.center.z)
                + building.size.y + 0.21f;
            const float z = building.center.z + building.size.z*0.5f + 0.14f;
            const float dx = building.size.x*0.40f;
            addString({building.center.x-dx,y,z},{building.center.x+dx,y,z},
                      building.blockId,5,0.075f);
        }

        // Spiral strings follow the current fir geometry and current safe positions.
        constexpr int treeBulbs = 28;
        constexpr float twoPi = 6.28318530718f;
        for (const auto& tree : parkFirs()) {
            const float ground = snowHeight(tree.x,tree.z);
            glm::vec3 previous{0.0f};
            for (int i=0;i<treeBulbs;++i) {
                const float t = static_cast<float>(i)/static_cast<float>(treeBulbs-1);
                const float localHeight = 0.58f + 2.48f*t;
                const float radius = 0.92f - 0.62f*t;
                const float angle = 0.55f + twoPi*2.7f*t;
                const glm::vec3 point{
                    tree.x + tree.scale*(radius+0.10f)*std::cos(angle),
                    ground + tree.scale*localHeight,
                    tree.z + tree.scale*(radius+0.10f)*std::sin(angle)};
                if (i>0) result.push_back({previous,point,tree.blockId,0,0.0f,false});
                result.push_back({point,point,tree.blockId,colorNumber%5,
                                  static_cast<float>(colorNumber)*0.87f,true,0.78f*tree.scale});
                ++colorNumber;
                previous=point;
            }
        }
        return result;
    }();
    return parts;
}

void drawHolidayGarlands(const Mesh& cube, unsigned int program,
                         const std::array<float,4>& blockLights,
                         bool enabled, double seconds) {
    const std::array<glm::vec3,5> palette{{
        {1.00f,0.70f,0.27f}, {1.00f,0.92f,0.69f}, {0.37f,0.84f,1.00f},
        {1.00f,0.54f,0.39f}, {0.77f,1.00f,0.73f}
    }};
    for (const auto& part : holidayGarlands()) {
        if (!part.isBulb) {
            cityBeam(cube,program,part.from,part.to,0.022f,{0.12f,0.16f,0.21f});
            continue;
        }
        const float district = std::clamp(blockLights[static_cast<std::size_t>(part.blockId-1)],0.0f,1.0f);
        const float glow = enabled ? district*(0.82f+0.18f*std::sin(static_cast<float>(seconds)*2.4f+part.phase)) : 0.0f;
        const glm::mat4 model = glm::translate(glm::mat4(1.0f),part.from)
            * glm::scale(glm::mat4(1.0f),glm::vec3(0.15f*part.size,0.18f*part.size,0.15f*part.size));
        const bool lit = glow > 0.015f;
        drawObject(cube,program,model,
                   lit ? palette[static_cast<std::size_t>(part.bulbColor)] : glm::vec3(0.06f,0.07f,0.09f),
                   false,lit,1.8f*glow);
    }
}

}

