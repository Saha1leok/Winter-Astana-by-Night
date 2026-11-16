#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Shader.h"
#include "River.h"
#include "Lighting.h"
#include "FrameCapture.h"
#include "SceneDirector.h"
#include "FireworkRenderer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int initialWidth = 1280;
constexpr int initialHeight = 720;
struct Mesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int indexCount = 0;

    Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
        indexCount = static_cast<int>(indices.size());
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }
};

using namespace frozenriver;

struct SnowParticle {
    glm::vec3 position;
    float speed;
    float drift;
};

int cameraMode = 0;  // Optional static camera for capture mode only.
bool showFrost = true;
bool showDrifts = true;
bool showGarlands = true;
// Master switch: L disables all architectural/decorative lights without
// overwriting the independent 1/2/G/H/B/K preferences or reveal timeline.
bool masterLightsOn = true;
// Controls for the three landmark illumination systems (independent of windows/garlands).
bool showKhanShatyrLighting = true;
bool showBayterekLighting = true;
bool showAkOrdaLighting = true;
// 0.65 = softer than the original full-strength landmark lighting.
// Change with - and = while running, or change this initial value.
float landmarkBrightness = 0.65f;
astana::SceneDirector* activeDirector = nullptr;
lighting::State lightState;
bool dragging = false;
double previousMouseX = 0.0;
double previousMouseY = 0.0;

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_1) lightState.windows = !lightState.windows;
    if (key == GLFW_KEY_2) lightState.lamps = !lightState.lamps;
    if (key == GLFW_KEY_L) masterLightsOn = !masterLightsOn;
    if (key == GLFW_KEY_F) showFrost = !showFrost;
    if (key == GLFW_KEY_D) showDrifts = !showDrifts;
    if (key == GLFW_KEY_G) showGarlands = !showGarlands;
    if (key == GLFW_KEY_H) showKhanShatyrLighting = !showKhanShatyrLighting;
    if (key == GLFW_KEY_B) showBayterekLighting = !showBayterekLighting;
    if (key == GLFW_KEY_K) showAkOrdaLighting = !showAkOrdaLighting;
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
        landmarkBrightness = std::clamp(landmarkBrightness - 0.10f, 0.20f, 1.0f);
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
        landmarkBrightness = std::clamp(landmarkBrightness + 0.10f, 0.20f, 1.0f);
    if (!activeDirector) return;
    if (key == GLFW_KEY_R || key == GLFW_KEY_SPACE) {
        dragging = false;
        if (key == GLFW_KEY_R) activeDirector->startReveal();
        else activeDirector->skipReveal();
    }
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void saveFrame(const std::filesystem::path& path, int width, int height) {
    if (width <= 0 || height <= 0) throw std::runtime_error("Cannot capture an empty framebuffer.");
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        file.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * width * 3), width * 3);
    }
    if (!file) throw std::runtime_error("Cannot save capture: " + path.string());
    std::cout << "Captured " << path << " (" << width << 'x' << height << ")\n";
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }
    dragging = action == GLFW_PRESS && activeDirector && activeDirector->orbitEnabled();
    if (dragging) {
        glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
    }
}

void cursorPositionCallback(GLFWwindow*, double x, double y) {
    if (!dragging || !activeDirector || !activeDirector->orbitEnabled()) return;
    const double deltaX = x - previousMouseX;
    const double deltaY = y - previousMouseY;
    previousMouseX = x;
    previousMouseY = y;
    activeDirector->drag(static_cast<float>(deltaX), static_cast<float>(deltaY));
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (activeDirector && activeDirector->orbitEnabled())
        activeDirector->zoom(static_cast<float>(yOffset));
}

std::filesystem::path shaderDirectory(const char* executablePath) {
    const std::filesystem::path local = std::filesystem::current_path() / "shaders";
    if (std::filesystem::exists(local / "scene.vert")) {
        return local;
    }
    const std::filesystem::path nextToExecutable = std::filesystem::absolute(executablePath).parent_path() / "shaders";
    if (std::filesystem::exists(nextToExecutable / "scene.vert")) {
        return nextToExecutable;
    }
    throw std::runtime_error("Shader folder not found. Run the program from the project folder or its build output folder.");
}

void setMatrix(unsigned int program, const char* name, const glm::mat4& matrix) {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

// Shared renderer for terrain (snow), windows/lamps (emission), and other meshes.
void drawObject(const Mesh& mesh, unsigned int program, const glm::mat4& model,
                const glm::vec3& color, bool ice, bool emissive = false,
                float emissionStrength = 1.0f, bool snow = false) {
    setMatrix(program, "uModel", model);
    glUniform3fv(glGetUniformLocation(program, "uBaseColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(program, "uIsIce"), ice ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), emissive ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uIsSnow"), snow ? 1 : 0);
    glUniform1f(glGetUniformLocation(program, "uEmissionStrength"), emissionStrength);
    mesh.draw();
}

// River and snow geometry are implemented in River.cpp (namespace frozenriver).

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


// Member 1: city layout and low-poly modeling.
// Coordinate convention: X crosses the river, Z runs along it, Y points up.
// Keep block IDs stable so Member 2 can connect the windows and lamps.
struct CityBuilding {
    int blockId;
    glm::vec3 center;  // ground X,Z; Y is unused
    glm::vec3 size;
    glm::vec3 facade;
    int style;  // 0 apartment, 1 modern office, 2 stepped tower
};

// Stable block IDs for Member 2. The river occupies the centre of the scene;
// districts 1/2 are west of it, and 3/4 are east of it.
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

// Stable anchors connect Member 2's lighting to the reveal block IDs.
struct StreetlightAnchor { int blockId; glm::vec3 position; };
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

// Low-poly radial meshes use height/radius profiles. Mesh objects are created
// only once at initialization; none are allocated during the render loop.
SurfaceData radialMesh(const std::vector<glm::vec2>& profile, int sides = 16) {
    SurfaceData result;
    constexpr float twoPi = 6.28318530718f;
    for (std::size_t ring = 0; ring < profile.size(); ++ring) {
        for (int j = 0; j <= sides; ++j) {
            const float a = twoPi * static_cast<float>(j) / static_cast<float>(sides);
            const float c = std::cos(a), si = std::sin(a);
            const float radius = profile[ring].y;
            result.vertices.insert(result.vertices.end(), {
                radius*c, profile[ring].x, radius*si, c, 0.25f, si
            });
        }
    }
    for (std::size_t i = 0; i + 1 < profile.size(); ++i) {
        for (int j = 0; j < sides; ++j) {
            const unsigned a = static_cast<unsigned>(i * (sides+1) + j);
            const unsigned b = a + static_cast<unsigned>(sides+1);
            result.indices.insert(result.indices.end(), {a,b,a+1,a+1,b,b+1});
        }
    }
    return result;
}

void drawRadial(const Mesh& mesh, unsigned int program, const glm::vec3& location,
                const glm::vec3& color, bool emissive = false) {
    drawObject(mesh,program,glm::translate(glm::mat4(1.0f),location),color,false,emissive);
}

// Road tiles sit above the terrain but away from the river. Keep clear spaces
// around the three monuments so the wide camera reveal remains readable.
void drawRoadTile(const Mesh& cube, unsigned int program, float x, float z,
                  float width, float depth) {
    const float y = snowHeight(x,z)+0.06f;
    cityBox(cube,program,{x,y,z},{width,0.09f,depth},{0.24f,0.30f,0.40f});
}

void drawCityRoads(const Mesh& cube, unsigned int program) {
    // Curved left embankment promenade with simple railings.
    for (float z=-11.2f; z<=10.4f; z+=0.75f) {
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
    // Window geometry is generated once by Member 2's lighting module below.
    // Keep Member 1's side bands and facade trim here.
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

void drawFirTree(const Mesh& cube, const Mesh& roof, unsigned int program, float x, float z, float scale = 1.0f) {
    const float y = snowHeight(x, z);
    cityBox(cube, program, {x, y + 0.28f * scale, z}, {0.18f * scale, 0.56f * scale, 0.18f * scale}, {0.25f, 0.18f, 0.10f});
    drawObject(roof, program, glm::translate(glm::mat4(1.0f), {x, y + 1.10f * scale, z}) * glm::scale(glm::mat4(1.0f), {0.38f * scale, 0.52f * scale, 0.38f * scale}), {0.10f, 0.22f, 0.14f}, false);
    drawObject(roof, program, glm::translate(glm::mat4(1.0f), {x, y + 0.78f * scale, z}) * glm::scale(glm::mat4(1.0f), {0.54f * scale, 0.50f * scale, 0.54f * scale}), {0.12f, 0.24f, 0.16f}, false);
    drawObject(roof, program, glm::translate(glm::mat4(1.0f), {x, y + 0.48f * scale, z}) * glm::scale(glm::mat4(1.0f), {0.70f * scale, 0.46f * scale, 0.70f * scale}), {0.12f, 0.26f, 0.18f}, false);
    cityBox(cube, program, {x, y + 1.26f * scale, z}, {0.18f * scale, 0.08f * scale, 0.18f * scale}, {0.92f, 0.95f, 1.0f});
    cityBox(cube, program, {x, y + 0.92f * scale, z}, {0.32f * scale, 0.10f * scale, 0.32f * scale}, {0.92f, 0.95f, 1.0f});
    cityBox(cube, program, {x, y + 0.60f * scale, z}, {0.46f * scale, 0.10f * scale, 0.46f * scale}, {0.92f, 0.95f, 1.0f});
}

// Shared tree placements keep holiday lights attached to the exact same firs
// that Member 1 draws. blockId links the decorations to the reveal timeline.
struct ParkFir { float x, z, scale; int blockId; };
const std::array<ParkFir, 12>& parkFirs() {
    static const std::array<ParkFir, 12> trees{{
        {-12.6f, 0.6f, 1.15f, 2}, {-11.2f, 1.6f, 0.95f, 2},
        { -9.6f, 2.8f, 1.05f, 2}, { -8.1f, 1.2f, 0.90f, 2},
        { -7.0f, 4.0f, 1.15f, 2}, { -5.8f, 0.9f, 0.85f, 2},
        { -4.6f,-0.6f, 0.95f, 2}, {  7.6f, 2.0f, 0.85f, 4},
        {  8.7f, 3.8f, 0.95f, 4}, { 10.0f, 2.4f, 1.05f, 4},
        { 11.5f, 1.6f, 0.90f, 4}, { 12.5f, 3.1f, 0.82f, 4}
    }};
    return trees;
}

void drawParkTrees(const Mesh& cube, const Mesh& roof, unsigned int program) {
    for (const auto& tree : parkFirs())
        drawFirTree(cube, roof, program, tree.x, tree.z, tree.scale);
}


// Architectural lighting: these emissive details follow the reveal timeline.
// They are deliberately low-poly; no extra point lights or post-processing needed.
void landmarkBox(const Mesh& cube, unsigned int program,
                 const glm::vec3& center, const glm::vec3& size,
                 const glm::vec3& unlit, const glm::vec3& lit,
                 float intensity, float strength = 1.8f) {
    const float t = std::clamp(intensity, 0.0f, 1.0f);
    const float glow = t * landmarkBrightness;
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), center)
        * glm::scale(glm::mat4(1.0f), size);
    drawObject(cube, program, model, glm::mix(unlit, lit, glow),
               false, t > 0.015f, 1.0f + glow * (strength - 1.0f));
}

void landmarkBeam(const Mesh& cube, unsigned int program,
                  const glm::vec3& from, const glm::vec3& to, float width,
                  const glm::vec3& unlit, const glm::vec3& lit,
                  float intensity, float strength = 1.7f) {
    const glm::vec3 direction = to - from;
    const float length = glm::length(direction);
    if (length < 0.001f) return;
    const glm::quat rotation = glm::rotation(glm::vec3(0, 1, 0), direction / length);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), (from + to) * 0.5f)
        * glm::mat4_cast(rotation)
        * glm::scale(glm::mat4(1.0f), glm::vec3(width, length, width));
    const float t = std::clamp(intensity, 0.0f, 1.0f);
    const float glow = t * landmarkBrightness;
    drawObject(cube, program, model, glm::mix(unlit, lit, glow),
               false, t > 0.015f, 1.0f + glow * (strength - 1.0f));
}

void landmarkRadial(const Mesh& mesh, unsigned int program,
                    const glm::vec3& center, const glm::vec3& unlit,
                    const glm::vec3& lit, float intensity, float strength = 1.8f) {
    const float t = std::clamp(intensity, 0.0f, 1.0f);
    const float glow = t * landmarkBrightness;
    drawObject(mesh, program, glm::translate(glm::mat4(1.0f), center),
               glm::mix(unlit, lit, glow), false, t > 0.015f,
               1.0f + glow * (strength - 1.0f));
}

void drawKhanShatyr(const Mesh& cube, const Mesh& roof,
                    unsigned int program, float light) {
    const float x = -10.7f, z = 1.8f, y = snowHeight(x, z);
    const glm::vec3 pearl{0.73f, 0.77f, 0.92f};
    const glm::vec3 blue{0.28f, 0.62f, 1.0f};
    const glm::vec3 violet{0.70f, 0.36f, 0.94f};

    cityBox(cube, program, {x,y+0.23f,z}, {4.65f,0.45f,4.30f}, {0.50f,0.52f,0.70f});
    cityBox(cube, program, {x,y+0.49f,z}, {4.86f,0.10f,4.55f}, {0.81f,0.83f,0.92f});
    landmarkRadial(roof, program, {x,y+0.51f,z}, {0.24f,0.23f,0.38f},
                   violet, light, 1.8f);

    constexpr float pi = 3.14159265358979f;
    for (int i = 0; i < 16; ++i) {
        const float angle = 2 * pi * i / 16;
        const glm::vec3 foot{x + 2.25f * std::cos(angle), y + 0.55f, z + 2.25f * std::sin(angle)};
        const glm::vec3 waist{x + 1.16f * std::cos(angle), y + 1.55f, z + 1.16f * std::sin(angle)};
        const glm::vec3 top{x + 0.10f * std::cos(angle), y + 4.9f, z + 0.10f * std::sin(angle)};
        landmarkBeam(cube, program, foot, waist, 0.065f, pearl, {0.91f,0.80f,1.0f}, light, 1.4f);
        landmarkBeam(cube, program, waist, top, 0.052f, pearl, {0.91f,0.80f,1.0f}, light, 1.4f);
    }

    // Low blue edge lighting: shallow strips on the outer base perimeter.
    const glm::vec3 darkEdge{0.28f,0.33f,0.48f};
    landmarkBox(cube, program, {x,y+0.57f,z-2.29f}, {4.80f,0.045f,0.07f},
                darkEdge, blue, light, 1.7f);
    landmarkBox(cube, program, {x,y+0.57f,z+2.29f}, {4.80f,0.045f,0.07f},
                darkEdge, blue, light, 1.7f);
    landmarkBox(cube, program, {x-2.46f,y+0.57f,z}, {0.07f,0.045f,4.48f},
                darkEdge, blue, light, 1.7f);
    landmarkBox(cube, program, {x+2.46f,y+0.57f,z}, {0.07f,0.045f,4.48f},
                darkEdge, blue, light, 1.7f);

    landmarkBeam(cube, program, {x,y+4.66f,z}, {x,y+6.16f,z},
                 0.075f, pearl, {0.98f,0.95f,1.0f}, light, 1.5f);
    cityBox(cube, program, {x,y+0.85f,z-2.21f}, {1.65f,0.83f,0.09f}, {0.22f,0.29f,0.46f});
    landmarkBox(cube, program, {x,y+1.28f,z-2.27f}, {1.58f,0.055f,0.07f},
                {0.37f,0.36f,0.48f}, {0.97f,0.91f,0.78f}, light, 1.6f);
}

void drawBayterek(const Mesh& cube, const Mesh& globe,
                 unsigned int program, float light) {
    const float x = 0.2f, z = 0.8f, y = snowHeight(x, z) + 0.17f;
    const glm::vec3 white{0.87f, 0.85f, 0.78f};
    const glm::vec3 gold{0.98f, 0.78f, 0.31f};
    cityBox(cube, program, {x,y+0.14f,z}, {2.75f,0.28f,2.75f}, {0.73f,0.78f,0.87f});
    cityBox(cube, program, {x,y+0.36f,z}, {1.95f,0.18f,1.95f}, {0.91f,0.91f,0.90f});
    landmarkBox(cube, program, {x,y+2.58f,z}, {0.50f,4.45f,0.50f},
                white, {0.96f,0.93f,0.80f}, light, 1.3f);

    constexpr float pi = 3.14159265358979f;
    for (int i = 0; i < 8; ++i) {
        const float a = 2 * pi * i / 8;
        const glm::vec3 unit{std::cos(a), 0, std::sin(a)};
        const glm::vec3 bottom = glm::vec3(x,y+0.45f,z) + unit * 0.44f;
        const glm::vec3 middle = glm::vec3(x,y+3.25f,z) + unit * 0.77f;
        const glm::vec3 upper = glm::vec3(x,y+5.05f,z) + unit * 1.12f;
        const glm::vec3 tip = glm::vec3(x,y+6.25f,z) + unit * 1.48f;
        landmarkBeam(cube, program, bottom, middle, 0.105f, white, {0.94f,0.90f,0.74f}, light, 1.35f);
        landmarkBeam(cube, program, middle, upper, 0.082f, white, {0.94f,0.90f,0.74f}, light, 1.45f);
        landmarkBeam(cube, program, upper, tip, 0.066f, gold, {1.0f,0.86f,0.46f}, light, 1.8f);
    }

    landmarkRadial(globe, program, {x,y+5.62f,z}, {0.43f,0.35f,0.22f},
                   gold, light, 2.15f);
    landmarkBox(cube, program, {x,y+4.35f,z}, {1.17f,0.15f,1.17f},
                {0.86f,0.70f,0.36f}, gold, light, 1.6f);
    for (float level : {1.05f,2.35f,3.65f}) {
        cityBox(cube, program, {x,y+level,z}, {1.08f,0.09f,1.08f},
                {0.77f,0.81f,0.85f});
    }
}

void drawAkOrda(const Mesh& cube, const Mesh& dome,
                unsigned int program, float light) {
    const float x = 11.6f, z = 1.9f, y = snowHeight(x, z);
    const glm::vec3 wall{0.86f,0.87f,0.92f};
    const glm::vec3 snow{0.96f,0.94f,0.92f};
    const glm::vec3 trim{0.70f,0.75f,0.85f};
    const glm::vec3 warm{1.0f,0.91f,0.70f};
    const glm::vec3 gold{0.98f,0.79f,0.32f};

    cityBox(cube, program, {x,y+0.17f,z}, {5.65f,0.34f,3.75f}, trim);
    cityBox(cube, program, {x,y+1.05f,z}, {5.35f,1.46f,3.15f}, wall);
    cityBox(cube, program, {x,y+1.83f,z}, {5.55f,0.16f,3.38f}, snow);
    cityBox(cube, program, {x,y+2.28f,z}, {4.6f,0.75f,2.56f}, wall);
    cityBox(cube, program, {x,y+2.71f,z}, {4.9f,0.14f,2.82f}, snow);
    cityBox(cube, program, {x,y+3.02f,z}, {2.00f,0.50f,1.85f}, wall);

    // Thin facade light strips, not whole glowing floors.
    landmarkBox(cube, program, {x,y+1.84f,z-1.73f}, {5.45f,0.065f,0.06f},
                trim, warm, light, 1.55f);
    landmarkBox(cube, program, {x,y+2.72f,z-1.44f}, {4.80f,0.055f,0.06f},
                trim, warm, light, 1.55f);

    cityBox(cube, program, {x,y+1.38f,z-1.78f}, {4.02f,0.15f,0.82f}, snow);
    for (int i = -3; i <= 3; ++i) {
        const float cx = x + i * 0.54f;
        landmarkBox(cube, program, {cx,y+0.93f,z-1.84f}, {0.135f,1.42f,0.14f},
                    snow, warm, light, 1.65f);
        cityBox(cube, program, {cx,y+1.66f,z-1.84f}, {0.25f,0.12f,0.25f}, trim);
        cityBox(cube, program, {cx,y+0.22f,z-1.84f}, {0.24f,0.12f,0.23f}, trim);
    }
    for (int i = -5; i <= 5; ++i) {
        const float wx = x + i * 0.45f;
        landmarkBox(cube, program, {wx,y+1.18f,z-1.592f}, {0.19f,0.37f,0.035f},
                    {0.31f,0.43f,0.63f}, {0.97f,0.79f,0.46f}, light, 1.65f);
    }
    cityBox(cube, program, {x,y+0.59f,z-1.95f}, {0.49f,0.95f,0.05f}, {0.32f,0.42f,0.58f});

    landmarkRadial(dome, program, {x,y+3.27f,z}, {0.20f,0.30f,0.46f},
                   {0.26f,0.58f,0.93f}, light, 1.65f);
    landmarkBeam(cube, program, {x,y+4.29f,z}, {x,y+5.18f,z},
                 0.105f, {0.98f,0.75f,0.30f}, gold, light, 1.8f);
    landmarkBox(cube, program, {x,y+5.23f,z}, {0.21f,0.17f,0.21f},
                {1.0f,0.82f,0.38f}, gold, light, 1.75f);
}

// Main Member 1 render entry point; no separate scene engine required.
void drawCityLayout(const Mesh& cube, const Mesh& khanRoof,
                    const Mesh& bayterekGlobe, const Mesh& akOrdaDome,
                    unsigned int program, const std::array<float, 4>& blockLights) {
    drawCityRoads(cube, program);
    for (const CityBuilding& b : cityBuildings()) drawDetailedBuilding(cube, program, b);
    drawParkTrees(cube, khanRoof, program);
    // Assign landmarks to the existing districts for ordered reveal.
    // A manual OFF overrides the reveal animation; ON resumes the current block fade.
    drawKhanShatyr(cube, khanRoof, program,
                   showKhanShatyrLighting ? blockLights[1] : 0.0f);
    drawBayterek(cube, bayterekGlobe, program,
                 showBayterekLighting ? blockLights[1] : 0.0f);
    drawAkOrda(cube, akOrdaDome, program,
               showAkOrdaLighting ? blockLights[3] : 0.0f);
}

// Festive details for the final reveal: draped garland strings and roofline bulbs.
// Small cubes match the existing low-poly style; no textures or new shaders needed.
struct GarlandPart {
    glm::vec3 from;
    glm::vec3 to;
    int blockId;
    int bulbColor;
    float phase;
    bool isBulb;
    float size = 1.0f; // tree bulbs are slightly smaller than bridge/roof bulbs
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
                glm::vec3 p = glm::mix(first, last, t);
                p.y -= 4.0f * sag * t * (1.0f - t);
                result.push_back({previous, p, blockId, 0, 0.0f, false});
                if (i <= bulbs) {
                    result.push_back({p, p, blockId, colorNumber % 5,
                                      static_cast<float>(colorNumber) * 0.87f, true});
                    ++colorNumber;
                }
                previous = p;
            }
        };

        // Promenade along the river: string lights run between the existing lamps.
        for (int i = 0; i < 10; ++i) {
            const float z0 = -10.0f + 2.0f * i;
            const float z1 = z0 + 2.0f;
            const float x0 = riverCenter(z0) - riverHalfWidth(z0) - 0.58f;
            const float x1 = riverCenter(z1) - riverHalfWidth(z1) - 0.58f;
            const glm::vec3 a(x0, snowHeight(x0, z0) + 1.61f, z0);
            const glm::vec3 b(x1, snowHeight(x1, z1) + 1.61f, z1);
            addString(a, b, z0 < -0.1f ? 1 : 2, 7, 0.20f);
        }

        // Two strings along the bridge railings, anchored to the bridge lamps.
        const float bridgeX = riverCenter(4.6f);
        for (float side : {-0.46f, 0.46f}) {
            for (int i = -1; i < 1; ++i) {
                const glm::vec3 a(bridgeX + static_cast<float>(i) * 1.55f, 1.52f, 4.6f + side);
                const glm::vec3 b(bridgeX + static_cast<float>(i + 1) * 1.55f, 1.52f, 4.6f + side);
                addString(a, b, 4, 6, 0.17f);
            }
        }

        // Short roofline garlands accent the real city buildings (no floating windows).
        for (const auto& building : cityBuildings()) {
            if (building.size.y < 4.0f) continue;
            const float y = snowHeight(building.center.x, building.center.z)
                          + building.size.y + 0.21f;
            const float z = building.center.z + building.size.z * 0.5f + 0.14f;
            const float dx = building.size.x * 0.40f;
            addString({building.center.x - dx, y, z},
                      {building.center.x + dx, y, z},
                      building.blockId, 5, 0.075f);
        }

        // Member 5: small spiral light chains around the existing 12 firs.
        // We sample the THREE actual low-poly canopy meshes (same roof profile
        // and transforms as drawFirTree) so bulbs sit just outside the branches.
        auto firRadiusAtHeight = [](float h) {
            const float profileH[4] = {0.0f, 1.10f, 3.00f, 4.40f};
            const float profileR[4] = {2.25f, 1.15f, 0.48f, 0.08f};
            const float tierY[3] = {0.48f, 0.78f, 1.10f};
            const float tierH[3] = {0.46f, 0.50f, 0.52f};
            const float tierR[3] = {0.70f, 0.54f, 0.38f};
            float outer = 0.0f;
            for (int tier = 0; tier < 3; ++tier) {
                const float meshY = (h - tierY[tier]) / tierH[tier];
                if (meshY < 0.0f || meshY > profileH[3]) continue;
                for (int j = 0; j < 3; ++j) {
                    if (meshY <= profileH[j + 1]) {
                        const float t = (meshY - profileH[j]) /
                                        (profileH[j + 1] - profileH[j]);
                        outer = std::max(outer, (profileR[j] +
                                     t * (profileR[j + 1] - profileR[j])) * tierR[tier]);
                        break;
                    }
                }
            }
            return outer;
        };
        constexpr int treeBulbs = 28;
        constexpr float twoPi = 6.28318530718f;
        for (const auto& tree : parkFirs()) {
            const float ground = snowHeight(tree.x, tree.z);
            glm::vec3 previous{0.0f};
            for (int i = 0; i < treeBulbs; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(treeBulbs - 1);
                const float localHeight = 0.81f + 1.73f * t;
                const float radius = firRadiusAtHeight(localHeight) + 0.13f;
                const float angle = 0.55f + twoPi * 2.7f * t;
                const glm::vec3 point{
                    tree.x + tree.scale * radius * std::cos(angle),
                    ground + tree.scale * localHeight,
                    tree.z + tree.scale * radius * std::sin(angle)
                };
                if (i > 0) result.push_back({previous, point, tree.blockId, 0, 0.0f, false});
                result.push_back({point, point, tree.blockId, colorNumber % 5,
                                  static_cast<float>(colorNumber) * 0.87f, true, 0.78f * tree.scale});
                ++colorNumber;
                previous = point;
            }
        }

        return result;
    }();
    return parts;
}

void drawHolidayGarlands(const Mesh& cube, unsigned int shaderProgram,
                         const std::array<float, 4>& blockLights,
                         bool enabled, double seconds) {
    const std::array<glm::vec3, 5> palette{{
        {1.00f, 0.70f, 0.27f}, // warm gold
        {1.00f, 0.92f, 0.69f}, // warm white
        {0.37f, 0.84f, 1.00f}, // icy blue
        {1.00f, 0.54f, 0.39f}, // gentle coral
        {0.77f, 1.00f, 0.73f}  // winter green
    }};
    for (const auto& part : holidayGarlands()) {
        if (!part.isBulb) {
            const glm::vec3 direction = part.to - part.from;
            if (glm::length(direction) > 0.001f)
                cityBeam(cube, shaderProgram, part.from, part.to, 0.022f,
                         {0.12f, 0.16f, 0.21f});
            continue;
        }
        const float district = std::clamp(blockLights[static_cast<std::size_t>(part.blockId - 1)], 0.0f, 1.0f);
        const float glow = enabled ? district *
            (0.82f + 0.18f * std::sin(static_cast<float>(seconds) * 2.4f + part.phase)) : 0.0f;
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), part.from)
            * glm::scale(glm::mat4(1.0f), glm::vec3(0.15f * part.size, 0.18f * part.size, 0.15f * part.size));
        const bool lit = glow > 0.015f;
        drawObject(cube, shaderProgram, model,
                   lit ? palette[static_cast<std::size_t>(part.bulbColor)] : glm::vec3(0.06f, 0.07f, 0.09f),
                   false, lit, 1.8f * glow);
    }
}

}

int main(int argc, char** argv) {
    std::filesystem::path captureDirectory;
    int frameLimit = 0;
    bool smokeTest = false;
    std::filesystem::path captures;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--smoke-test" && i + 1 < argc) {
            smokeTest = true;
            captures = argv[++i];
        } else if (argument == "--capture-dir" && i + 1 < argc) {
            captureDirectory = argv[++i];
        } else if (argument == "--view" && i + 1 < argc) {
            const std::string view = argv[++i];
            if (view == "orbit") cameraMode = 0;
            else if (view == "street") cameraMode = 1;
            else if (view == "above") cameraMode = 2;
            else { std::cerr << "--view requires orbit, street, or above.\n"; return EXIT_FAILURE; }
        } else if (argument == "--frames" && i + 1 < argc) {
            try { frameLimit = std::stoi(argv[++i]); } catch (...) { frameLimit = -1; }
            if (frameLimit <= 0) { std::cerr << "--frames requires a positive integer.\n"; return EXIT_FAILURE; }
        } else {
            std::cerr << "Usage: winter_astana [--smoke-test DIR] [--capture-dir DIR] [--frames COUNT] [--view orbit|street|above]\n";
            return EXIT_FAILURE;
        }
    }
    if (smokeTest && !captureDirectory.empty()) {
        std::cerr << "Use --smoke-test or --capture-dir, not both.\n";
        return EXIT_FAILURE;
    }
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Keep the performance target and captures at 1280x720 on Retina displays too.
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    if (smokeTest) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(initialWidth, initialHeight, "Winter Astana - Week 7", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glGetError();
    // Cocoa may finish applying the non-Retina backing size on its first event.
    glfwPollEvents();

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    int result = EXIT_SUCCESS;
    try {
        if (!captureDirectory.empty()) std::filesystem::create_directories(captureDirectory);
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\nOpenGL: " << glGetString(GL_VERSION) << '\n';
        const std::filesystem::path shaderPath = shaderDirectory(argc > 0 ? argv[0] : "winter_astana");
        Shader shader((shaderPath / "scene.vert").string(), (shaderPath / "scene.frag").string());
        Shader snowShader((shaderPath / "snow.vert").string(), (shaderPath / "snow.frag").string());
        astana::FireworkRenderer fireworkRenderer(shaderPath);
        astana::Fireworks fireworks;
        std::array<float, 4> blockLights{{1, 1, 1, 1}};
        astana::RevealHooks hooks;
        hooks.setBlockLight = [&](int id, float intensity) { blockLights.at(static_cast<std::size_t>(id - 1)) = intensity; };
        hooks.launchFirework = [&](const astana::FireworkCue& cue, double age) { fireworks.launch(cue, age); };
        hooks.resetFireworks = [&] { fireworks.reset(); };
        hooks.orbitRestored = [] { dragging = false; };
        astana::SceneDirector director({}, hooks);
        activeDirector = &director;
        director.startReveal();
        if (smokeTest) std::filesystem::create_directories(captures);
        std::cout << "R: replay reveal | Space: skip | Drag: orbit | Wheel: zoom\n"
                  << "1: windows | 2: lamps | L: ALL light master switch | G: garlands | Esc: exit\n"
                  << "H: Khan Shatyr | B: Bayterek | K: Ak Orda | - / =: landmark brightness\n";

        const std::vector<unsigned int> cubeIndices{
             0, 1, 2, 2, 3, 0,  4, 5, 6, 6, 7, 4,  8, 9,10,10,11, 8,
            12,13,14,14,15,12, 16,17,18,18,19,16, 20,21,22,22,23,20
        };
        const SurfaceData leftBankData = createSnowBank(true);
        const SurfaceData rightBankData = createSnowBank(false);
        const SurfaceData riverData = createRiver();
        const SurfaceData leftWallData = createBankWall(true);
        const SurfaceData rightWallData = createBankWall(false);
        const SurfaceData leftDriftData = createSnowDrift(true);
        const SurfaceData rightDriftData = createSnowDrift(false);
        Mesh leftBank(leftBankData.vertices, leftBankData.indices);
        Mesh rightBank(rightBankData.vertices, rightBankData.indices);
        Mesh river(riverData.vertices, riverData.indices);
        Mesh leftWall(leftWallData.vertices, leftWallData.indices);
        Mesh rightWall(rightWallData.vertices, rightWallData.indices);
        Mesh leftDrift(leftDriftData.vertices, leftDriftData.indices);
        Mesh rightDrift(rightDriftData.vertices, rightDriftData.indices);
        Mesh cube(cubeVertices(), cubeIndices);
        // Landmark roofs: original low-poly radial geometry.
        const SurfaceData khanData = radialMesh({{0.0f,2.25f},{1.1f,1.15f},{3.00f,0.48f},{4.40f,0.08f}},16);
        const SurfaceData globeData = radialMesh({{-0.95f,0.05f},{-0.82f,0.55f},{-0.50f,0.85f},{0.0f,1.0f},{0.50f,0.85f},{0.82f,0.55f},{0.95f,0.05f}},16);
        const SurfaceData domeData = radialMesh({{0.0f,1.05f},{0.24f,1.0f},{0.7f,0.78f},{1.0f,0.4f},{1.1f,0.04f}},16);
        Mesh khanRoof(khanData.vertices,khanData.indices);
        Mesh bayterekGlobe(globeData.vertices,globeData.indices);
        Mesh akOrdaDome(domeData.vertices,domeData.indices);

        // The city layout in cityBuildings() is the single source of truth.
        // Do not create lights for legacy background boxes that are no longer rendered.

        struct BlockWindow { int blockId; lighting::Window pane; };
        struct BlockLamp { int blockId; lighting::Lamp lamp; bool localLight = true; };
        std::vector<BlockWindow> windows;
        std::vector<BlockLamp> lamps;
        std::array<glm::vec3, 4> skylineProbes{};
        auto addWindows = [&](int blockId, glm::vec3 center, glm::vec3 size, std::uint32_t seed) {
            for (const auto& pane : lighting::buildingWindows(center, size, seed))
                windows.push_back({blockId, pane});
        };
        for (std::size_t i = 0; i < cityBuildings().size(); ++i) {
            const auto& b = cityBuildings()[i];
            const float base = snowHeight(b.center.x, b.center.z);
            addWindows(b.blockId, b.center + glm::vec3(0, base + b.size.y * 0.5f, 0), b.size, 42u + static_cast<std::uint32_t>(i));
            auto& probe = skylineProbes[static_cast<std::size_t>(b.blockId - 1)];
            if (base + b.size.y > probe.y) probe = b.center + glm::vec3(0, base + b.size.y, 0);
        }
        for (const auto& anchor : cityStreetlightAnchors()) {
            const glm::vec3 base = anchor.position + glm::vec3(0, snowHeight(anchor.position.x, anchor.position.z) + 0.10f, 0);
            lamps.push_back({anchor.blockId, {base, base + glm::vec3(0, 1.53f, 0)}});
        }
        // Preserve the original promenade and all six bridge lamps. The eight
        // block anchors plus four bridge bulbs supply Member 2's 12 point lights;
        // the remaining bulbs still follow their block's emissive fade.
        for (float z : {-10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f}) {
            const float x = riverCenter(z) - riverHalfWidth(z) - 0.58f;
            const glm::vec3 base(x, snowHeight(x, z) + 0.10f, z);
            lamps.push_back({z < 0 ? 1 : 2, {base, base + glm::vec3(0, 1.53f, 0)}, false});
        }
        for (float side : {-0.46f, 0.46f}) {
            for (int i = -1; i <= 1; ++i) {
                const glm::vec3 base(riverCenter(4.6f) + i * 1.55f, 0.45f, 4.6f + side);
                lamps.push_back({4, {base, base + glm::vec3(0, 1.07f, 0)}, i != 0});
            }
        }
        const auto pointLights = std::count_if(lamps.begin(), lamps.end(), [](const auto& lamp) { return lamp.localLight; });
        if (pointLights > lighting::maxLamps) throw std::runtime_error("City exceeds Member 2's lamp budget");
        std::vector<lighting::Lamp> activeLamps;
        std::vector<float> lampStrengths;
        activeLamps.reserve(lamps.size());
        lampStrengths.reserve(lamps.size());

        std::mt19937 randomGenerator(7);
        std::uniform_real_distribution<float> xDistribution(-13.0f, 13.0f);
        std::uniform_real_distribution<float> yDistribution(0.5f, 12.0f);
        std::uniform_real_distribution<float> zDistribution(-12.0f, 12.0f);
        std::uniform_real_distribution<float> speedDistribution(0.8f, 2.2f);
        std::uniform_real_distribution<float> driftDistribution(0.4f, 1.4f);
        std::vector<SnowParticle> snowParticles(650);
        std::vector<glm::vec3> snowPositions(snowParticles.size());
        for (SnowParticle& particle : snowParticles) {
            particle.position = {xDistribution(randomGenerator), yDistribution(randomGenerator), zDistribution(randomGenerator)};
            particle.speed = speedDistribution(randomGenerator);
            particle.drift = driftDistribution(randomGenerator);
        }

        unsigned int snowVao = 0;
        unsigned int snowVbo = 0;
        glGenVertexArrays(1, &snowVao);
        glGenBuffers(1, &snowVbo);
        glBindVertexArray(snowVao);
        glBindBuffer(GL_ARRAY_BUFFER, snowVbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(snowPositions.size() * sizeof(glm::vec3)), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glEnable(GL_PROGRAM_POINT_SIZE);

        double lastFpsUpdate = glfwGetTime();
        double previousFrameTime = lastFpsUpdate;
        int frameCount = 0;
        int totalFrames = 0;
        const double startTime = lastFpsUpdate;
        const std::array<const char*, 5> captureNames{{"overview.ppm", "street.ppm", "above.ppm", "above-no-frost.ppm", "above-no-drifts.ppm"}};
        int testFrame = 0;

        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            const double currentFrameTime = glfwGetTime();
            const double frameDelta = smokeTest ? (testFrame == 0 ? 0.0 : 1.0 / 60.0)
                : (!captureDirectory.empty() ? 0.0 : std::max(0.0, currentFrameTime - previousFrameTime));
            const float deltaTime = static_cast<float>(std::min(frameDelta, 0.05));
            previousFrameTime = currentFrameTime;
            if (!captureDirectory.empty()) {
                cameraMode = std::min(totalFrames, 2);
                showFrost = totalFrames != 3;
                showDrifts = totalFrames != 4;
            }
            if (director.orbitEnabled()) {
                const float orbitX = static_cast<float>((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                    - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS));
                const float orbitY = static_cast<float>((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                    - (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS));
                if (orbitX != 0 || orbitY != 0)
                    director.drag(orbitX * 90.0f * deltaTime, -orbitY * 70.0f * deltaTime);
            }
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            const float aspect = framebufferWidth > 0 && framebufferHeight > 0
                ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 1.0f;
            const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
            const auto beforeUpdate = director.pose();
            const auto visibilityMatrix = projection * glm::lookAt(beforeUpdate.position, beforeUpdate.target, glm::vec3(0, 1, 0));
            std::array<bool, 4> visibleBlocks{};
            for (std::size_t i = 0; i < visibleBlocks.size(); ++i)
                visibleBlocks[i] = astana::skylineVisible(visibilityMatrix, skylineProbes[i]);
            fireworks.update(frameDelta);
            director.update(frameDelta, visibleBlocks);
            // Keep the reveal timeline running in the background, but prevent
            // ALL its light fades from reaching the renderer while L is OFF.
            std::array<float, 4> renderedBlockLights = blockLights;
            if (!masterLightsOn) renderedBlockLights.fill(0.0f);
            const auto camera = director.pose();
            glm::vec3 cameraPosition = camera.position;
            glm::vec3 cameraTarget = camera.target;
            // Static capture views, without overriding the cinematic reveal in normal playback.
            if (!captureDirectory.empty() && cameraMode == 1) {
                const float x = riverCenter(9.0f) + riverHalfWidth(9.0f) + 0.75f;
                cameraPosition = glm::vec3(x, snowHeight(x, 9.0f) + 1.60f, 9.0f);
                cameraTarget = glm::vec3(riverCenter(-2.0f), 0.055f, -2.0f);
            } else if (!captureDirectory.empty() && cameraMode == 2) {
                cameraPosition = glm::vec3(3.0f, 32.0f, 12.0f);
                cameraTarget = glm::vec3(0.5f, 0.0f, 0.0f);
            }
            const glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));

            glClearColor(0.025f, 0.045f, 0.095f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shader.use();
            setMatrix(shader.id(), "uView", view);
            setMatrix(shader.id(), "uProjection", projection);
            glUniform3fv(glGetUniformLocation(shader.id(), "uCameraPosition"), 1, glm::value_ptr(cameraPosition));
            glUniform1i(glGetUniformLocation(shader.id(), "uShowFrost"), showFrost ? 1 : 0);
            activeLamps.clear();
            lampStrengths.clear();
            for (const auto& grouped : lamps) {
                const float strength = renderedBlockLights[static_cast<std::size_t>(grouped.blockId - 1)];
                if (grouped.localLight && lightState.lamps && strength > 0.0f) {
                    activeLamps.push_back(grouped.lamp);
                    lampStrengths.push_back(strength);
                }
            }
            // Keep Member 2's public API unchanged; upload per-block fade weights
            // alongside its compact array of active lamps.
            lighting::uploadLamps(shader.id(), activeLamps, masterLightsOn && lightState.lamps);
            if (!lampStrengths.empty()) glUniform1fv(glGetUniformLocation(shader.id(), "uLampStrengths[0]"),
                static_cast<GLsizei>(lampStrengths.size()), lampStrengths.data());

            drawObject(leftBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, 1.0f, true);
            drawObject(rightBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, 1.0f, true);
            drawObject(river, shader.id(), glm::mat4(1.0f), {0.10f, 0.29f, 0.50f}, true);
            drawObject(leftWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);
            drawObject(rightWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);

            if (showDrifts) {
                drawObject(leftDrift, shader.id(), glm::mat4(1.0f), {0.85f, 0.91f, 0.98f}, false, false, 1.0f, true);
                drawObject(rightDrift, shader.id(), glm::mat4(1.0f), {0.85f, 0.91f, 0.98f}, false, false, 1.0f, true);
            }

            // Updated composition: closer to the visual reference while keeping the same winter rendering pipeline.
            drawCityLayout(cube,khanRoof,bayterekGlobe,akOrdaDome,shader.id(),renderedBlockLights);

            for (const auto& grouped : windows) {
                const auto& pane = grouped.pane;
                const float intensity = lightState.windows && pane.occupied
                    ? renderedBlockLights[static_cast<std::size_t>(grouped.blockId - 1)] : 0.0f;
                const auto model = glm::translate(glm::mat4(1.0f), pane.position)
                    * glm::scale(glm::mat4(1.0f), pane.size);
                drawObject(cube, shader.id(), model, intensity > 0.0f ? pane.color : glm::vec3(0.025f, 0.045f, 0.075f),
                    false, intensity > 0.0f, intensity);
            }
            for (const auto& grouped : lamps) {
                const auto& lamp = grouped.lamp;
                const float intensity = lightState.lamps ? renderedBlockLights[static_cast<std::size_t>(grouped.blockId - 1)] : 0.0f;
                const float height = lamp.bulb.y - lamp.base.y;
                const glm::mat4 postModel = glm::translate(glm::mat4(1.0f), lamp.base + glm::vec3(0, height * 0.47f, 0))
                    * glm::scale(glm::mat4(1.0f), {0.10f, height * 0.94f, 0.10f});
                drawObject(cube, shader.id(), postModel, {0.055f, 0.065f, 0.085f}, false);
                const glm::mat4 bulbModel = glm::translate(glm::mat4(1.0f), lamp.bulb)
                    * glm::scale(glm::mat4(1.0f), {0.28f, 0.24f, 0.28f});
                drawObject(cube, shader.id(), bulbModel, intensity > 0.0f ? glm::vec3(1.0f, 0.70f, 0.28f) : glm::vec3(0.08f),
                    false, intensity > 0.0f, intensity);
            }

            // Garlands follow each district as the camera reveals the skyline.
            drawHolidayGarlands(cube, shader.id(), renderedBlockLights, showGarlands, currentFrameTime);

            for (std::size_t i = 0; i < snowParticles.size(); ++i) {
                SnowParticle& particle = snowParticles[i];
                particle.position.y -= particle.speed * deltaTime;
                particle.position.x += std::sin(static_cast<float>(currentFrameTime) * 0.7f + particle.position.z) * particle.drift * deltaTime * 0.18f;
                if (particle.position.y < 0.15f) {
                    particle.position.y = 11.5f;
                    particle.position.x = xDistribution(randomGenerator);
                    particle.position.z = zDistribution(randomGenerator);
                }
                snowPositions[i] = particle.position;
            }

            glBindBuffer(GL_ARRAY_BUFFER, snowVbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(snowPositions.size() * sizeof(glm::vec3)), snowPositions.data());
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            snowShader.use();
            setMatrix(snowShader.id(), "uView", view);
            setMatrix(snowShader.id(), "uProjection", projection);
            glBindVertexArray(snowVao);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(snowPositions.size()));
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);

            fireworkRenderer.draw(fireworks, view, projection);
            if (!captureDirectory.empty() && totalFrames < static_cast<int>(captureNames.size())) {
                saveFrame(captureDirectory / captureNames[totalFrames], framebufferWidth, framebufferHeight);
            }
            const GLenum error = glGetError();
            if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
            if (smokeTest) {
                const std::array<int, 5> frames{{0, 240, 420, 660, 900}};
                const std::array<const char*, 5> names{{"start.bmp", "block-one.bmp", "skyline-rise.bmp", "fireworks.bmp", "orbit-restored.bmp"}};
                for (std::size_t i = 0; i < frames.size(); ++i) {
                    if (testFrame == frames[i]) {
                        captureBmp(captures / names[i], framebufferWidth, framebufferHeight);
                        std::cout << "PASS " << names[i] << '\n';
                    }
                }
                if (testFrame == 660 && fireworks.vertices().empty())
                    throw std::runtime_error("Finale did not render firework particles");
                if (testFrame == 900) {
                    if (!director.orbitEnabled()) throw std::runtime_error("Orbit control was not restored");
                    for (float light : blockLights) if (light < 0.99f) throw std::runtime_error("Block was not revealed");
                    keyCallback(window, GLFW_KEY_R, 0, GLFW_REPEAT, 0);
                    if (!director.orbitEnabled()) throw std::runtime_error("Key repeat restarted the reveal");
                    const auto before = director.pose();
                    mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                    cursorPositionCallback(window, previousMouseX + 20, previousMouseY + 5);
                    mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
                    scrollCallback(window, 0, 1);
                    if (glm::length(director.pose().position - before.position) < 0.1f)
                        throw std::runtime_error("Mouse orbit/zoom did not change the camera");
                    keyCallback(window, GLFW_KEY_1, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_2, 0, GLFW_PRESS, 0);
                    if (lightState.windows || lightState.lamps) throw std::runtime_error("Lighting toggles failed");
                    // Master OFF must not alter individual settings, and must
                    // suppress every light group even during the reveal.
                    keyCallback(window, GLFW_KEY_L, 0, GLFW_PRESS, 0);
                    if (masterLightsOn || lightState.windows || lightState.lamps)
                        throw std::runtime_error("Master lights OFF failed");
                    keyCallback(window, GLFW_KEY_R, 0, GLFW_PRESS, 0);
                    if (masterLightsOn) throw std::runtime_error("Reveal overrode master lights OFF");
                    keyCallback(window, GLFW_KEY_L, 0, GLFW_PRESS, 0);
                    if (!masterLightsOn || lightState.windows || lightState.lamps)
                        throw std::runtime_error("Master lights ON did not preserve preferences");
                    if (director.orbitEnabled() || !fireworks.vertices().empty()) throw std::runtime_error("Replay did not reset");
                    keyCallback(window, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
                    if (!director.orbitEnabled()) throw std::runtime_error("Skip did not restore orbit");
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                const auto error = glGetError();
                if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
                ++testFrame;
            }
            glfwSwapBuffers(window);
            glfwPollEvents();

            ++frameCount;
            ++totalFrames;
            if ((!captureDirectory.empty() && totalFrames == static_cast<int>(captureNames.size()))
                || (frameLimit > 0 && totalFrames >= frameLimit)) glfwSetWindowShouldClose(window, GLFW_TRUE);
            const double now = glfwGetTime();
            const double elapsed = now - lastFpsUpdate;
            if (elapsed >= 0.5) {
                const double fps = static_cast<double>(frameCount) / elapsed;
                std::ostringstream title;
                title << "Winter Astana | " << director.phaseName() << " | R Replay | Space Skip"
                      << " | L All lights " << (masterLightsOn ? "ON" : "OFF")
                      << " | 1 Windows " << (masterLightsOn && lightState.windows ? "ON" : "OFF")
                      << " | 2 Lamps " << (masterLightsOn && lightState.lamps ? "ON" : "OFF")
                      << " | F Frost " << (showFrost ? "ON" : "OFF")
                      << " | D Drifts " << (showDrifts ? "ON" : "OFF")
                      << " | G Garlands " << (masterLightsOn && showGarlands ? "ON" : "OFF")
                      << " | H " << (masterLightsOn && showKhanShatyrLighting ? "ON" : "OFF")
                      << " | B " << (masterLightsOn && showBayterekLighting ? "ON" : "OFF")
                      << " | K " << (masterLightsOn && showAkOrdaLighting ? "ON" : "OFF")
                      << " | Landmark " << static_cast<int>(landmarkBrightness * 100.0f + 0.5f) << "%"
                      << " | FPS: " << std::fixed << std::setprecision(1) << fps;
                glfwSetWindowTitle(window, title.str().c_str());
                frameCount = 0;
                lastFpsUpdate = now;
            }
        }
        std::cout << "Rendered " << totalFrames << " frames; average " << totalFrames / (glfwGetTime() - startTime)
                  << " FPS; OpenGL checks passed.\n";
        glDeleteVertexArrays(1, &snowVao);
        glDeleteBuffers(1, &snowVbo);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = EXIT_FAILURE;
    }

    activeDirector = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}

