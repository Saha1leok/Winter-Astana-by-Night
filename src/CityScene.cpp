#define GLM_ENABLE_EXPERIMENTAL
#include "CityScene.h"

#include "Controls.h"
#include "Mesh.h"
#include "RenderUtils.h"
#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace astana {

const std::vector<CityBuilding>& cityBuildings() {
    static const std::vector<CityBuilding> buildings{
        {1, {-12.2f,0,-9.4f}, {1.8f,3.8f,1.7f}, {0.39f,0.43f,0.57f},0},
        {1, { -9.8f,0,-9.2f}, {1.9f,5.3f,1.8f}, {0.31f,0.39f,0.56f},1},
        {1, { -7.4f,0,-9.1f}, {1.8f,4.4f,1.7f}, {0.43f,0.44f,0.58f},0},
        {1, {-11.3f,0,-6.6f}, {1.7f,3.2f,1.5f}, {0.42f,0.45f,0.61f},0},
        {1, { -8.6f,0,-6.4f}, {1.8f,4.2f,1.8f}, {0.32f,0.38f,0.56f},2},
        {1, { -6.0f,0,-6.3f}, {1.6f,3.0f,1.5f}, {0.49f,0.44f,0.57f},0},
        {2, {-2.6f,0,-10.3f}, {1.6f,5.7f,1.5f}, {0.24f,0.26f,0.37f},2},
        {2, { 0.2f,0,-9.9f},  {1.8f,4.0f,1.6f}, {0.35f,0.40f,0.55f},0},
        {2, { 2.7f,0,-10.1f}, {1.6f,6.4f,1.6f}, {0.18f,0.29f,0.46f},1},
        {2, { 5.3f,0,-10.0f}, {1.7f,6.9f,1.7f}, {0.20f,0.31f,0.49f},2},
        {2, { 7.8f,0,-9.7f},  {1.8f,5.1f,1.7f}, {0.28f,0.37f,0.52f},1},
        {2, {10.3f,0,-9.5f},  {1.8f,4.1f,1.7f}, {0.37f,0.41f,0.56f},0},
        {2, {-1.4f,0,-5.8f}, {1.8f,2.3f,1.5f}, {0.53f,0.50f,0.55f},0},
        {2, { 1.2f,0,-5.9f}, {1.8f,2.3f,1.5f}, {0.54f,0.51f,0.57f},0},
        {2, { 3.8f,0,-5.8f}, {1.8f,2.4f,1.5f}, {0.52f,0.50f,0.56f},0},
        {3, { 8.8f,0,-6.1f}, {1.7f,3.8f,1.6f}, {0.40f,0.42f,0.58f},0},
        {3, {11.4f,0,-6.0f}, {1.8f,4.9f,1.8f}, {0.29f,0.37f,0.53f},1},
        {3, {13.6f,0,-5.8f}, {1.6f,3.7f,1.5f}, {0.37f,0.40f,0.55f},0},
        {3, {12.8f,0,-9.2f}, {1.7f,5.6f,1.7f}, {0.28f,0.39f,0.54f},2},
        {4, { 9.7f,0,-12.0f}, {1.5f,3.2f,1.4f}, {0.39f,0.43f,0.59f},0},
        {4, {12.2f,0,-11.8f}, {1.6f,4.0f,1.5f}, {0.29f,0.37f,0.53f},1},
        {4, {14.4f,0,-11.7f}, {1.4f,3.0f,1.4f}, {0.40f,0.42f,0.56f},0}
    };
    return buildings;
}

const std::vector<StreetlightAnchor>& cityStreetlightAnchors() {
    static const std::vector<StreetlightAnchor> anchors{
        {1,{-7.4f,0,-7.3f}}, {1,{-10.3f,0,-7.3f}},
        {2,{-7.4f,0,6.9f}}, {2,{-10.6f,0,6.9f}},
        {3,{ 7.0f,0,-7.2f}}, {3,{10.1f,0,-7.2f}},
        {4,{ 7.2f,0,7.6f}}, {4,{11.4f,0,7.6f}}
    };
    return anchors;
}

void cityBox(const Mesh& cube, unsigned int program, const glm::vec3& center,
             const glm::vec3& size, const glm::vec3& color) {
    const glm::mat4 m = glm::translate(glm::mat4(1.0f), center)
        * glm::scale(glm::mat4(1.0f), size);
    drawObject(cube, program, m, color, false);
}

void cityBeam(const Mesh& cube, unsigned int program, const glm::vec3& from,
              const glm::vec3& to, float width, const glm::vec3& color) {
    glm::vec3 direction = to - from;
    const float len = glm::length(direction);
    if (len < 0.001f) return;
    const glm::quat rot = glm::rotation(glm::vec3(0,1,0), direction / len);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), (from+to)*0.5f)
        * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), glm::vec3(width,len,width));
    drawObject(cube,program,model,color,false);
}

// Road tiles sit above the terrain but away from the river. Keep clear spaces
// around the three monuments so the wide camera reveal remains readable.
void drawRoadTile(const Mesh& cube, unsigned int program, float x, float z,
                  float width, float depth) {
    const float y = snowHeight(x,z)+0.035f;
    // Pale, thin winter paving reads as a road instead of a hard black shadow
    // where the tiles intersect the uneven snow surface.
    cityBox(cube,program,{x,y,z},{width,0.055f,depth},{0.48f,0.55f,0.66f});
}

void drawCityRoads(const Mesh& cube, unsigned int program) {
    // Curved left embankment promenade with simple railings.
    for (float z=-15.2f; z<=18.2f; z+=0.75f) {
        const float edge = riverCenter(z) - riverHalfWidth(z);
        const float x = edge - 1.35f;
        const float y = snowHeight(x, z) + 0.08f;
        cityBox(cube, program, {x, y, z}, {1.55f, 0.12f, 0.62f}, {0.72f, 0.75f, 0.82f});
        cityBox(cube, program, {edge - 0.20f, y + 0.26f, z}, {0.04f, 0.34f, 0.04f}, {0.08f, 0.10f, 0.14f});
        cityBox(cube, program, {edge - 0.46f, y + 0.26f, z}, {0.04f, 0.34f, 0.04f}, {0.08f, 0.10f, 0.14f});
        cityBox(cube, program, {edge - 0.33f, y + 0.38f, z}, {0.30f, 0.03f, 0.03f}, {0.08f, 0.10f, 0.14f});
    }
    // Right embankment near Ak Orda.
    for (float z=-1.0f; z<=7.8f; z+=0.75f) {
        const float edge = riverCenter(z) + riverHalfWidth(z);
        const float x = edge + 0.95f;
        const float y = snowHeight(x, z) + 0.08f;
        cityBox(cube, program, {x, y, z}, {1.15f, 0.12f, 0.60f}, {0.72f, 0.75f, 0.82f});
    }
    // Central Bayterek plaza.
    const float py = snowHeight(-0.5f, 0.5f);
    cityBox(cube, program, {-0.5f, py+0.10f, 0.5f}, {5.6f, 0.16f, 4.2f}, {0.69f, 0.74f, 0.83f});
    cityBox(cube, program, {-0.5f, py+0.18f, 0.5f}, {3.4f, 0.08f, 2.6f}, {0.80f, 0.83f, 0.90f});
    // Simple streets behind the monuments.
    for (float x=-12.0f; x<=13.5f; x+=0.8f) drawRoadTile(cube, program, x, -7.4f, 0.80f, 0.86f);
    for (float x=-12.0f; x<=13.5f; x+=0.8f) drawRoadTile(cube, program, x, -4.2f, 0.80f, 0.86f);
    for (float x=-12.0f; x<=13.5f; x+=0.8f) drawRoadTile(cube, program, x,  8.2f, 0.80f, 0.86f);
    for (float z=-10.8f; z<=9.6f; z+=0.8f) {
        drawRoadTile(cube, program, -9.0f, z, 0.85f, 0.82f);
        drawRoadTile(cube, program,  8.9f, z, 0.85f, 0.82f);
    }
    // Low-poly bridge on the right.
    const float bz = 4.6f;
    const float bc = riverCenter(bz);
    const float span = riverHalfWidth(bz) + 1.0f;
    cityBox(cube, program, {bc, 0.38f, bz}, {span * 2.2f, 0.20f, 1.25f}, {0.64f, 0.66f, 0.74f});
    cityBox(cube, program, {bc, 0.62f, bz - 0.55f}, {span * 2.1f, 0.08f, 0.10f}, {0.82f, 0.84f, 0.89f});
    cityBox(cube, program, {bc, 0.62f, bz + 0.55f}, {span * 2.1f, 0.08f, 0.10f}, {0.82f, 0.84f, 0.89f});
    cityBox(cube, program, {bc - span * 0.55f, 0.17f, bz}, {0.58f, 0.34f, 0.94f}, {0.43f, 0.45f, 0.54f});
    cityBox(cube, program, {bc + span * 0.55f, 0.17f, bz}, {0.58f, 0.34f, 0.94f}, {0.43f, 0.45f, 0.54f});
    const glm::vec3 bridgeMetal{0.11f, 0.13f, 0.18f};
    for (float x = bc - span; x <= bc + span; x += 0.52f) {
        for (float side : {-0.58f, 0.58f})
            cityBox(cube, program, {x, 0.78f, bz + side}, {0.055f, 0.42f, 0.055f}, bridgeMetal);
    }
    for (float side : {-0.58f, 0.58f}) {
        cityBox(cube, program, {bc, 0.90f, bz + side}, {span * 2.08f, 0.055f, 0.055f}, bridgeMetal);
        cityBox(cube, program, {bc, 0.72f, bz + side}, {span * 2.08f, 0.035f, 0.035f}, bridgeMetal);
    }
    // Faceted braces suggest the two shallow arches visible under the bridge.
    for (float direction : {-1.0f, 1.0f}) {
        const glm::vec3 pier{bc + direction * span * 0.55f, 0.12f, bz};
        cityBeam(cube, program, pier, {bc + direction * span * 0.12f, 0.34f, bz}, 0.11f, {0.35f, 0.37f, 0.45f});
        cityBeam(cube, program, pier, {bc + direction * span * 0.96f, 0.34f, bz}, 0.11f, {0.35f, 0.37f, 0.45f});
    }
}

void drawDetailedBuilding(const Mesh& cube, unsigned int program, const CityBuilding& b) {
    const float x=b.center.x,z=b.center.z;
    const float base=snowHeight(x,z);
    const glm::vec3 sz=b.size;
    cityBox(cube,program,{x,base+sz.y*0.5f,z},sz,b.facade);
    const glm::vec3 trim={0.67f,0.73f,0.85f};
    // Snow-capped cornice and differentiated upper silhouette.
    cityBox(cube,program,{x,base+sz.y+0.075f,z},
            {sz.x+0.20f,0.15f,sz.z+0.20f},trim);
    if(b.style==1) {
        cityBox(cube,program,{x,base+sz.y+0.25f,z},
                {sz.x*0.67f,0.37f,sz.z*0.64f},{0.26f,0.38f,0.53f});
        cityBox(cube,program,{x,base+sz.y+0.47f,z},
                {sz.x*0.72f,0.08f,sz.z*0.69f},{0.29f,0.68f,0.79f});
    } else if(b.style==2) {
        cityBox(cube,program,{x,base+sz.y+0.38f,z},
                {sz.x*0.58f,0.7f,sz.z*0.58f},b.facade*0.85f);
        cityBox(cube,program,{x,base+sz.y+0.78f,z},
                {sz.x*0.66f,0.12f,sz.z*0.66f},trim);
    } else {
        cityBox(cube,program,{x,base+0.14f,z-sz.z*0.51f},
                {sz.x*0.42f,0.28f,0.23f},{0.58f,0.66f,0.76f});
    }
    const int floors=std::max(2,static_cast<int>(sz.y/0.7f));
    for(int floor=0; floor<floors; ++floor) {
        const float y=base+0.42f+float(floor)*0.69f;
        if(y>base+sz.y-0.17f) break;
        // Light architectural bands on side elevations.
        if(floor%2==0) {
            cityBox(cube,program,{x+sz.x*0.502f,y,z},
                    {0.025f,0.055f,sz.z*0.91f},{0.44f,0.53f,0.67f});
        }
    }
    for(float dx : {-sz.x*0.40f, sz.x*0.40f}) {
        cityBox(cube,program,{x+dx,base+sz.y*0.5f,z+sz.z*0.51f},
                {0.055f,sz.y,0.055f},{0.56f,0.62f,0.72f});
    }
}

void drawFirTree(const Mesh& cube, const Mesh& crown, const Mesh& snowCap,
                 unsigned int program, float x, float z, float scale, float snowCoverage) {
    const float y = snowHeight(x, z);
    cityBox(cube, program, {x, y + 0.52f * scale, z}, {0.20f * scale, 1.04f * scale, 0.20f * scale}, {0.24f, 0.16f, 0.09f});
    const glm::vec3 foliage{0.055f, 0.19f, 0.115f};
    auto cone = [&](float baseY, float radius, float height, const glm::vec3& color) {
        drawObject(crown, program,
            glm::translate(glm::mat4(1.0f), {x, y + baseY * scale, z})
                * glm::scale(glm::mat4(1.0f), {radius * scale, height * scale, radius * scale}),
            color, false);
    };
    // Three separate branch cones form a recognizable fir, not a scaled Khan Shatyr.
    cone(0.28f, 1.00f, 1.55f, foliage);
    cone(1.03f, 0.78f, 1.48f, foliage * 1.08f);
    cone(1.78f, 0.55f, 1.42f, foliage * 1.16f);

    // Separate snow geometry grows over the branches; foliage never changes color.
    const float phase = 0.5f + 0.5f * std::sin(x * 1.71f + z * 1.29f);
    float localCoverage = std::clamp(snowCoverage * 1.35f - phase * 0.35f, 0.0f, 1.0f);
    localCoverage = localCoverage * localCoverage * (3.0f - 2.0f * localCoverage);
    if (localCoverage > 0.01f) {
        const glm::vec3 snow{0.88f, 0.93f, 1.0f};
        auto cap = [&](float baseY, float radius, float height) {
            const float spread = 0.22f + 0.78f * localCoverage;
            drawObject(snowCap, program,
                glm::translate(glm::mat4(1.0f), {x, y + baseY * scale, z})
                    * glm::scale(glm::mat4(1.0f), {
                        radius * scale * spread,
                        height * scale * std::max(localCoverage, 0.08f),
                        radius * scale * spread}),
                snow, false);
        };
        cap(1.04f, 0.82f, 0.48f);
        cap(1.77f, 0.62f, 0.44f);
        cap(2.47f, 0.43f, 0.40f);
    }
}

const std::array<ParkFir, 12>& parkFirs() {
    static const std::array<ParkFir, 12> trees{{
        // These positions are shared by the tree meshes and their garlands.
        {-13.8f, 3.8f, 0.92f, 2}, {-12.3f, 5.5f, 0.78f, 2},
        {-10.5f, 4.7f, 0.84f, 2}, { -8.1f, 4.6f, 0.76f, 2},
        { -6.4f, 4.1f, 0.88f, 2}, { -5.2f, 2.9f, 0.72f, 2},
        { -4.2f, 4.7f, 0.78f, 2}, {  7.0f, 4.8f, 0.72f, 4},
        {  8.6f, 5.8f, 0.78f, 4}, { 10.4f, 5.2f, 0.84f, 4},
        { 12.4f, 5.9f, 0.76f, 4}, { 14.4f, 4.6f, 0.70f, 4}
    }};
    return trees;
}

void drawParkTrees(const Mesh& cube, const Mesh& crown, const Mesh& snowCap,
                   unsigned int program, float snowCoverage) {
    for (const auto& tree : parkFirs())
        drawFirTree(cube, crown, snowCap, program, tree.x, tree.z, tree.scale, snowCoverage);
}

void drawBench(const Mesh& cube, unsigned int program, float x, float z) {
    const float y = snowHeight(x, z);
    const glm::vec3 wood{0.31f, 0.18f, 0.10f};
    const glm::vec3 metal{0.055f, 0.065f, 0.08f};
    cityBox(cube, program, {x, y + 0.40f, z}, {0.48f, 0.10f, 1.18f}, wood);
    cityBox(cube, program, {x - 0.19f, y + 0.70f, z}, {0.10f, 0.52f, 1.18f}, wood);
    for (float dz : {-0.42f, 0.42f}) {
        cityBox(cube, program, {x - 0.12f, y + 0.20f, z + dz}, {0.10f, 0.42f, 0.10f}, metal);
        cityBox(cube, program, {x + 0.12f, y + 0.20f, z + dz}, {0.10f, 0.42f, 0.10f}, metal);
    }
}

void drawPerson(const Mesh& cube, const Mesh& head, unsigned int program,
                const glm::vec3& position, const glm::vec3& coat) {
    const float y = snowHeight(position.x, position.z);
    const glm::vec3 dark{0.045f, 0.055f, 0.075f};
    cityBox(cube, program, {position.x, y + 0.78f, position.z}, {0.32f, 0.72f, 0.25f}, coat);
    cityBox(cube, program, {position.x - 0.10f, y + 0.26f, position.z}, {0.10f, 0.52f, 0.12f}, dark);
    cityBox(cube, program, {position.x + 0.10f, y + 0.26f, position.z}, {0.10f, 0.52f, 0.12f}, dark);
    drawObject(head, program,
        glm::translate(glm::mat4(1.0f), {position.x, y + 1.27f, position.z})
            * glm::scale(glm::mat4(1.0f), glm::vec3(0.15f)),
        {0.76f, 0.58f, 0.46f}, false);
    cityBox(cube, program, {position.x, y + 1.43f, position.z}, {0.24f, 0.10f, 0.24f}, dark);
}

void drawStreetFurniture(const Mesh& cube, const Mesh& head, unsigned int program) {
    for (float z : {-4.8f, 0.2f, 5.2f}) {
        const float edge = riverCenter(z) - riverHalfWidth(z);
        drawBench(cube, program, edge - 1.82f, z);
    }
    drawPerson(cube, head, program, {-2.4f, 0.0f, 1.8f}, {0.50f, 0.16f, 0.18f});
    drawPerson(cube, head, program, {-1.8f, 0.0f, 2.2f}, {0.16f, 0.30f, 0.52f});
    drawPerson(cube, head, program, { 5.8f, 0.0f, 5.8f}, {0.42f, 0.25f, 0.52f});
    drawPerson(cube, head, program, { 6.3f, 0.0f, 6.1f}, {0.18f, 0.42f, 0.35f});
}

void landmarkObject(const Mesh& mesh, unsigned int program, const glm::mat4& model,
                    const glm::vec3& unlit, const glm::vec3& lit,
                    float intensity, float strength = 1.8f, float emissionBase = 1.0f) {
    const float t = std::clamp(intensity, 0.0f, 1.0f);
    const float glow = t * inputState().landmarkBrightness;
    drawObject(mesh, program, model, glm::mix(unlit, lit, glow), false,
               t > 0.015f, emissionBase + glow * (strength - emissionBase));
}

void landmarkBox(const Mesh& cube, unsigned int program,
                 const glm::vec3& center, const glm::vec3& size,
                 const glm::vec3& unlit, const glm::vec3& lit,
                 float intensity, float strength = 1.8f, float emissionBase = 1.0f) {
    landmarkObject(cube, program,
        glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), size),
        unlit, lit, intensity, strength, emissionBase);
}

void landmarkBeam(const Mesh& cube, unsigned int program,
                  const glm::vec3& from, const glm::vec3& to, float width,
                  const glm::vec3& unlit, const glm::vec3& lit,
                  float intensity, float strength = 1.7f, float emissionBase = 1.0f) {
    const glm::vec3 direction = to - from;
    const float length = glm::length(direction);
    if (length < 0.001f) return;
    const glm::quat rotation = glm::rotation(glm::vec3(0, 1, 0), direction / length);
    landmarkObject(cube, program,
        glm::translate(glm::mat4(1.0f), (from + to) * 0.5f)
            * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.0f), glm::vec3(width, length, width)),
        unlit, lit, intensity, strength, emissionBase);
}

void drawKhanShatyr(const Mesh& cube,const Mesh& roof,unsigned int program,float light) {
    const float x=-9.35f,z=0.2f,y=snowHeight(x,z);
    const glm::vec3 pearl={0.73f,0.77f,0.92f};
    cityBox(cube,program,{x,y+0.23f,z},{4.65f,0.45f,4.30f},{0.50f,0.52f,0.70f});
    cityBox(cube,program,{x,y+0.49f,z},{4.86f,0.10f,4.55f},{0.81f,0.83f,0.92f});
    landmarkObject(roof, program, glm::translate(glm::mat4(1.0f), {x,y+0.51f,z}),
                   {0.24f,0.23f,0.38f}, {0.70f,0.36f,0.94f}, light, 1.8f);
    // Visible tent ribs converge at the high crown, rather than a plain cone.
    constexpr float pi=3.14159265358979f;
    for(int i=0;i<16;i++) {
        const float angle=2*pi*i/16;
        const glm::vec3 foot={x+2.25f*std::cos(angle),y+0.55f,z+2.25f*std::sin(angle)};
        const glm::vec3 waist={x+1.16f*std::cos(angle),y+1.55f,z+1.16f*std::sin(angle)};
        const glm::vec3 top={x+0.10f*std::cos(angle),y+4.9f,z+0.10f*std::sin(angle)};
        landmarkBeam(cube,program,foot,waist,0.065f,pearl,{0.91f,0.80f,1.0f},light,1.4f);
        landmarkBeam(cube,program,waist,top,0.052f,pearl,{0.91f,0.80f,1.0f},light,1.4f);
    }
    const glm::vec3 darkEdge{0.28f,0.33f,0.48f};
    const glm::vec3 blue{0.28f,0.62f,1.0f};
    landmarkBox(cube,program,{x,y+0.57f,z-2.29f},{4.80f,0.045f,0.07f},darkEdge,blue,light,1.7f);
    landmarkBox(cube,program,{x,y+0.57f,z+2.29f},{4.80f,0.045f,0.07f},darkEdge,blue,light,1.7f);
    landmarkBox(cube,program,{x-2.46f,y+0.57f,z},{0.07f,0.045f,4.48f},darkEdge,blue,light,1.7f);
    landmarkBox(cube,program,{x+2.46f,y+0.57f,z},{0.07f,0.045f,4.48f},darkEdge,blue,light,1.7f);
    landmarkBeam(cube,program,{x,y+4.66f,z},{x,y+6.16f,z},0.075f,
                 {0.89f,0.91f,0.99f},{0.98f,0.95f,1.0f},light,1.5f);
    cityBox(cube,program,{x,y+0.85f,z-2.21f},{1.65f,0.83f,0.09f},{0.22f,0.29f,0.46f});
}

void drawBayterek(const Mesh& cube,const Mesh& globe,unsigned int program,float light) {
    const float x=0.2f,z=0.8f,y=snowHeight(x,z)+0.17f;
    const float softLight = light * 0.72f;
    const glm::vec3 white={0.78f,0.77f,0.72f};
    const glm::vec3 gold={0.86f,0.70f,0.36f};
    cityBox(cube,program,{x,y+0.14f,z},{2.75f,0.28f,2.75f},{0.73f,0.78f,0.87f});
    cityBox(cube,program,{x,y+0.36f,z},{1.95f,0.18f,1.95f},{0.91f,0.91f,0.90f});
    landmarkBox(cube,program,{x,y+3.35f,z},{0.54f,5.95f,0.54f},
                white,{0.94f,0.90f,0.76f},softLight,1.18f,0.38f);
    // Eight outward-curving columns forming a basket around the sphere.
    constexpr float pi=3.14159265358979f;
    for(int i=0;i<12;i++) {
        const float a=2*pi*i/12;
        const glm::vec3 unit={std::cos(a),0,std::sin(a)};
        const glm::vec3 bottom=glm::vec3(x,y+0.45f,z)+unit*0.44f;
        const glm::vec3 middle=glm::vec3(x,y+4.15f,z)+unit*0.82f;
        const glm::vec3 upper=glm::vec3(x,y+6.15f,z)+unit*1.20f;
        const glm::vec3 tip=glm::vec3(x,y+7.65f,z)+unit*1.55f;
        landmarkBeam(cube,program,bottom,middle,0.105f,white,{0.94f,0.90f,0.76f},softLight,1.18f,0.34f);
        landmarkBeam(cube,program,middle,upper,0.082f,white,{0.94f,0.90f,0.76f},softLight,1.22f,0.34f);
        landmarkBeam(cube,program,upper,tip,0.066f,gold,{0.98f,0.80f,0.41f},softLight,1.38f,0.38f);
    }
    // Three polygonal basket rings make the crown read as a constructed landmark.
    for (float level : {5.35f, 6.25f, 7.15f}) {
        const float radius = 0.95f + (level - 5.35f) * 0.27f;
        for (int i = 0; i < 12; ++i) {
            const float a = 2*pi*i/12;
            const float b = 2*pi*(i+1)/12;
            landmarkBeam(cube, program,
                {x + radius*std::cos(a), y + level, z + radius*std::sin(a)},
                {x + radius*std::cos(b), y + level, z + radius*std::sin(b)},
                0.045f, white, {0.94f,0.86f,0.64f}, softLight, 1.18f, 0.34f);
        }
    }
    landmarkObject(globe,program,
        glm::translate(glm::mat4(1.0f),{x,y+7.05f,z})
            * glm::scale(glm::mat4(1.0f),glm::vec3(1.12f)),
        {0.42f,0.33f,0.20f},{0.96f,0.72f,0.27f},softLight,1.55f,0.45f);
    landmarkBox(cube,program,{x,y+5.45f,z},{1.26f,0.16f,1.26f},
                gold,{0.98f,0.82f,0.43f},softLight,1.28f,0.36f);
    for(float level:{1.25f,2.85f,4.45f}) {
        cityBox(cube,program,{x,y+level,z},{1.08f,0.09f,1.08f},{0.77f,0.81f,0.85f});
    }
}

void drawAkOrda(const Mesh& cube,const Mesh& dome,unsigned int program,float light) {
    const float x=10.45f,z=0.4f,y=snowHeight(x,z);
    const glm::vec3 wall={0.86f,0.87f,0.92f};
    const glm::vec3 snow={0.96f,0.94f,0.92f};
    const glm::vec3 trim={0.70f,0.75f,0.85f};
    const glm::vec3 warm={1.0f,0.91f,0.70f};
    const glm::vec3 gold={0.98f,0.79f,0.32f};
    cityBox(cube,program,{x,y+0.17f,z},{5.65f,0.34f,3.75f},trim);
    cityBox(cube,program,{x,y+1.05f,z},{5.35f,1.46f,3.15f},wall);
    cityBox(cube,program,{x,y+1.83f,z},{5.55f,0.16f,3.38f},snow);
    cityBox(cube,program,{x,y+2.28f,z},{4.6f,0.75f,2.56f},wall);
    cityBox(cube,program,{x,y+2.71f,z},{4.9f,0.14f,2.82f},snow);
    cityBox(cube,program,{x,y+3.02f,z},{2.00f,0.50f,1.85f},wall);
    landmarkBox(cube,program,{x,y+1.84f,z-1.73f},{5.45f,0.065f,0.06f},trim,warm,light,1.55f);
    landmarkBox(cube,program,{x,y+2.72f,z-1.44f},{4.80f,0.055f,0.06f},trim,warm,light,1.55f);
    // Monumental colonnaded portico facing the camera and the river.
    cityBox(cube,program,{x,y+1.38f,z-1.78f},{4.02f,0.15f,0.82f},snow);
    for(int i=-3;i<=3;i++) {
        const float cx=x+i*0.54f;
        landmarkBox(cube,program,{cx,y+0.93f,z-1.84f},{0.135f,1.42f,0.14f},
                    snow,warm,light,1.65f);
        cityBox(cube,program,{cx,y+1.66f,z-1.84f},{0.25f,0.12f,0.25f},trim);
        cityBox(cube,program,{cx,y+0.22f,z-1.84f},{0.24f,0.12f,0.23f},trim);
    }
    for(int i=-5;i<=5;i++) {
        const float wx=x+i*0.45f;
        landmarkBox(cube,program,{wx,y+1.18f,z-1.592f},{0.19f,0.37f,0.035f},
                    {0.31f,0.43f,0.63f},{0.97f,0.79f,0.46f},light,1.65f);
    }
    cityBox(cube,program,{x,y+0.59f,z-1.95f},{0.49f,0.95f,0.05f},
            {0.32f,0.42f,0.58f});
    landmarkObject(dome,program,glm::translate(glm::mat4(1.0f),{x,y+3.27f,z}),
                   {0.20f,0.30f,0.46f},{0.26f,0.58f,0.93f},light,1.65f);
    landmarkBeam(cube,program,{x,y+4.29f,z},{x,y+5.18f,z},0.105f,
                 {0.98f,0.75f,0.30f},gold,light,1.8f);
    landmarkBox(cube,program,{x,y+5.23f,z},{0.21f,0.17f,0.21f},
                {1.0f,0.82f,0.38f},gold,light,1.75f);
}

void drawCityLayout(const Mesh& cube,const Mesh& khanRoof,const Mesh& firCrown,const Mesh& firSnowCap,
                    const Mesh& bayterekGlobe,const Mesh& akOrdaDome,
                    unsigned int program,float snowCoverage,
                    const std::array<float,4>& blockLights) {
    drawCityRoads(cube,program);
    for(const CityBuilding& b:cityBuildings()) drawDetailedBuilding(cube,program,b);
    drawParkTrees(cube,firCrown,firSnowCap,program,snowCoverage);
    drawKhanShatyr(cube,khanRoof,program,inputState().showKhanShatyrLighting ? blockLights[1] : 0.0f);
    drawBayterek(cube,bayterekGlobe,program,inputState().showBayterekLighting ? blockLights[1] : 0.0f);
    drawAkOrda(cube,akOrdaDome,program,inputState().showAkOrdaLighting ? blockLights[3] : 0.0f);
    drawStreetFurniture(cube,bayterekGlobe,program);
}


}

