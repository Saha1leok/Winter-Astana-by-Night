#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CityScene.h"
#include "Controls.h"
#include "Decorations.h"
#include "FireworkRenderer.h"
#include "FrameCapture.h"
#include "Lighting.h"
#include "Mesh.h"
#include "RenderUtils.h"
#include "SceneDirector.h"
#include "SceneNode.h"
#include "Shader.h"
#include "Terrain.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
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

struct SnowParticle {
    glm::vec3 position;
    float speed;
    float drift;
};

std::filesystem::path shaderDirectory(const char* executablePath) {
    const std::filesystem::path local = std::filesystem::current_path() / "shaders";
    if (std::filesystem::exists(local / "scene.vert")) return local;
    const auto nextToExecutable = std::filesystem::absolute(executablePath).parent_path() / "shaders";
    if (std::filesystem::exists(nextToExecutable / "scene.vert")) return nextToExecutable;
    throw std::runtime_error("Shader folder not found. Run from the project or build folder.");
}

}

using namespace astana;

int main(int argc, char** argv) {
    const bool smokeTest = argc == 3 && std::string(argv[1]) == "--smoke-test";
    if (argc != 1 && !smokeTest) {
        std::cerr << "Usage: winter_astana [--smoke-test capture-directory]\n";
        return EXIT_FAILURE;
    }
    const std::filesystem::path captures = smokeTest ? argv[2] : "";
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (smokeTest) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(initialWidth, initialHeight, "Winter Astana by Night", nullptr, nullptr);
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

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    int result = EXIT_SUCCESS;
    try {
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
        hooks.orbitRestored = [] { inputState().dragging = false; };
        astana::SceneDirector director({}, hooks);
        inputState().director = &director;
        director.startReveal();
        if (smokeTest) std::filesystem::create_directories(captures);
        std::cout << "R: replay reveal | Space: skip | Drag: orbit | Wheel: zoom\n"
                  << "1: windows | 2: lamps | L: all lights | G: garlands\n"
                  << "H: Khan Shatyr | B: Bayterek | K: Ak Orda | -/+: brightness | Esc: exit\n";

        const std::vector<unsigned int> cubeIndices{
             0, 1, 2, 2, 3, 0,  4, 5, 6, 6, 7, 4,  8, 9,10,10,11, 8,
            12,13,14,14,15,12, 16,17,18,18,19,16, 20,21,22,22,23,20
        };
        const SurfaceData leftBankData = createSnowBank(true);
        const SurfaceData rightBankData = createSnowBank(false);
        const SurfaceData riverData = createRiver();
        const SurfaceData leftWallData = createBankWall(true);
        const SurfaceData rightWallData = createBankWall(false);
        Mesh leftBank(leftBankData.vertices, leftBankData.indices);
        Mesh rightBank(rightBankData.vertices, rightBankData.indices);
        Mesh river(riverData.vertices, riverData.indices);
        Mesh leftWall(leftWallData.vertices, leftWallData.indices);
        Mesh rightWall(rightWallData.vertices, rightWallData.indices);
        Mesh cube(cubeVertices(), cubeIndices);
        const SurfaceData khanData = radialMesh({{0.0f,2.25f},{1.1f,1.15f},{3.00f,0.48f},{4.40f,0.08f}},16);
        const SurfaceData firData = radialMesh({{0.0f,1.0f},{1.0f,0.0f}},12);
        const SurfaceData firSnowData = radialMesh({
            {0.00f,1.00f},{0.10f,1.06f},{0.24f,0.72f},{0.48f,0.0f}},12);
        const SurfaceData globeData = radialMesh({{-0.95f,0.05f},{-0.82f,0.55f},{-0.50f,0.85f},{0.0f,1.0f},{0.50f,0.85f},{0.82f,0.55f},{0.95f,0.05f}},16);
        const SurfaceData domeData = radialMesh({{0.0f,1.05f},{0.24f,1.0f},{0.7f,0.78f},{1.0f,0.4f},{1.1f,0.04f}},16);
        Mesh khanRoof(khanData.vertices,khanData.indices);
        Mesh firCrown(firData.vertices,firData.indices);
        Mesh firSnowCap(firSnowData.vertices,firSnowData.indices);
        Mesh bayterekGlobe(globeData.vertices,globeData.indices);
        Mesh akOrdaDome(domeData.vertices,domeData.indices);

        const std::array<glm::vec3, 6> positions{{
            {-13.0f, 2.2f, -8.6f}, {-10.6f, 2.9f, -11.6f}, {-6.6f, 2.4f, -11.7f},
            { 6.6f, 2.3f, -11.5f}, { 9.0f, 3.0f, -11.8f}, {13.8f, 2.1f, -8.8f}
        }};
        const std::array<glm::vec3, 6> scales{{
            {1.9f, 2.3f, 1.7f}, {1.8f, 3.7f, 1.6f}, {1.9f, 2.8f, 1.7f},
            {1.9f, 2.8f, 1.7f}, {1.8f, 4.2f, 1.6f}, {2.0f, 2.2f, 1.7f}
        }};
        const std::array<glm::vec3, 6> colors{{
            {0.34f,0.39f,0.52f}, {0.42f,0.48f,0.62f}, {0.38f,0.42f,0.55f},
            {0.46f,0.40f,0.55f}, {0.35f,0.45f,0.58f}, {0.41f,0.43f,0.57f}
        }};

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
        const std::array<int, 6> backgroundBlocks{{1, 1, 1, 2, 3, 4}};
        for (std::size_t i = 0; i < positions.size(); ++i)
            addWindows(backgroundBlocks[i], positions[i], scales[i], 140u + static_cast<std::uint32_t>(i));
        for (const auto& anchor : cityStreetlightAnchors()) {
            const glm::vec3 base = anchor.position + glm::vec3(0, snowHeight(anchor.position.x, anchor.position.z) + 0.10f, 0);
            lamps.push_back({anchor.blockId, {base, base + glm::vec3(0, 1.53f, 0)}});
        }
        // Promenade and bridge lamps share the same block reveal timing.
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
        if (pointLights > lighting::maxLamps)
            throw std::runtime_error("City exceeds the supported point-light count");
        std::vector<lighting::Lamp> activeLamps;
        std::vector<float> lampStrengths;
        activeLamps.reserve(lamps.size());
        lampStrengths.reserve(lamps.size());

        std::mt19937 randomGenerator(7);
        std::uniform_real_distribution<float> xDistribution(-18.0f, 18.0f);
        std::uniform_real_distribution<float> yDistribution(0.5f, 12.0f);
        std::uniform_real_distribution<float> zDistribution(-16.0f, 19.0f);
        std::uniform_real_distribution<float> speedDistribution(0.8f, 2.2f);
        std::uniform_real_distribution<float> driftDistribution(0.4f, 1.4f);
        std::vector<SnowParticle> snowParticles(1100);
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
        double previousDirectorElapsed = director.elapsed();
        double snowAccumulationStart = -1.0;
        int frameCount = 0;
        int testFrame = 0;

        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            const float aspect = framebufferWidth > 0 && framebufferHeight > 0
                ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 1.0f;
            const double currentFrameTime = glfwGetTime();
            const double frameDelta = smokeTest ? (testFrame == 0 ? 0.0 : 1.0 / 60.0)
                : std::max(0.0, currentFrameTime - previousFrameTime);
            const float deltaTime = static_cast<float>(std::min(frameDelta, 0.05));
            previousFrameTime = currentFrameTime;
            const glm::mat4 projection = glm::perspective(glm::radians(49.0f), aspect, 0.1f, 100.0f);
            const auto beforeUpdate = director.pose();
            const auto visibilityMatrix = projection * glm::lookAt(beforeUpdate.position, beforeUpdate.target, glm::vec3(0, 1, 0));
            std::array<bool, 4> visibleBlocks{};
            for (std::size_t i = 0; i < visibleBlocks.size(); ++i)
                visibleBlocks[i] = astana::skylineVisible(visibilityMatrix, skylineProbes[i]);
            fireworks.update(frameDelta);
            director.update(frameDelta, visibleBlocks);
            const double directorElapsed = director.elapsed();
            if (directorElapsed + 0.0001 < previousDirectorElapsed)
                snowAccumulationStart = -1.0; // Replay resets accumulated snow.
            if (snowAccumulationStart < 0.0 && director.orbitEnabled() && fireworks.vertices().empty())
                snowAccumulationStart = directorElapsed;
            previousDirectorElapsed = directorElapsed;
            const auto camera = director.pose();
            const glm::vec3 cameraPosition = camera.position;
            const glm::mat4 view = glm::lookAt(cameraPosition, camera.target, glm::vec3(0, 1, 0));

            glClearColor(0.025f, 0.045f, 0.095f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shader.use();
            setMatrix(shader.id(), "uView", view);
            setMatrix(shader.id(), "uProjection", projection);
            glUniform3fv(glGetUniformLocation(shader.id(), "uCameraPosition"), 1, glm::value_ptr(cameraPosition));
            const float snowCoverage = snowAccumulationStart < 0.0 ? 0.0f
                : std::clamp(static_cast<float>((directorElapsed - snowAccumulationStart) / 6.0), 0.0f, 1.0f);
            glUniform1f(glGetUniformLocation(shader.id(), "uSnowCoverage"), snowCoverage);
            activeLamps.clear();
            lampStrengths.clear();
            for (const auto& grouped : lamps) {
                const float strength = blockLights[static_cast<std::size_t>(grouped.blockId - 1)];
                if (grouped.localLight && inputState().lights.lamps && strength > 0.0f) {
                    activeLamps.push_back(grouped.lamp);
                    lampStrengths.push_back(strength);
                }
            }
            lighting::uploadLamps(shader.id(), activeLamps, inputState().lights.lamps);
            if (!lampStrengths.empty()) glUniform1fv(glGetUniformLocation(shader.id(), "uLampStrengths[0]"),
                static_cast<GLsizei>(lampStrengths.size()), lampStrengths.data());

            drawObject(leftBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, 1.0f, true);
            drawObject(rightBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, 1.0f, true);
            drawObject(river, shader.id(), glm::mat4(1.0f), {0.055f, 0.16f, 0.34f}, true);
            drawObject(leftWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);
            drawObject(rightWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);

            for (std::size_t i = 0; i < positions.size(); ++i) {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]) * glm::scale(glm::mat4(1.0f), scales[i]);
                drawObject(cube, shader.id(), model, colors[i], false);
            }

            drawCityLayout(cube,khanRoof,firCrown,firSnowCap,bayterekGlobe,akOrdaDome,
                shader.id(),snowCoverage,blockLights);
            drawHolidayGarlands(cube,shader.id(),blockLights,inputState().showGarlands,currentFrameTime);

            for (const auto& grouped : windows) {
                const auto& pane = grouped.pane;
                const float intensity = inputState().lights.windows && pane.occupied
                    ? blockLights[static_cast<std::size_t>(grouped.blockId - 1)] : 0.0f;
                const auto model = glm::translate(glm::mat4(1.0f), pane.position)
                    * glm::scale(glm::mat4(1.0f), pane.size);
                drawObject(cube, shader.id(), model, intensity > 0.0f ? pane.color : glm::vec3(0.025f, 0.045f, 0.075f),
                    false, intensity > 0.0f, intensity);
            }
            for (const auto& grouped : lamps) {
                const auto& lamp = grouped.lamp;
                const float intensity = inputState().lights.lamps ? blockLights[static_cast<std::size_t>(grouped.blockId - 1)] : 0.0f;
                const float height = lamp.bulb.y - lamp.base.y;
                const SceneNode lampRoot(glm::translate(glm::mat4(1.0f), lamp.base));
                const SceneNode bulbNode(glm::translate(glm::mat4(1.0f), {0.0f, height, 0.0f}), &lampRoot);
                const glm::mat4 postModel = lampRoot.worldTransform()
                    * glm::translate(glm::mat4(1.0f), {0.0f, height * 0.47f, 0.0f})
                    * glm::scale(glm::mat4(1.0f), {0.075f, height * 0.94f, 0.075f});
                drawObject(cube, shader.id(), postModel, {0.055f, 0.065f, 0.085f}, false);
                drawObject(cube, shader.id(), lampRoot.worldTransform()
                    * glm::translate(glm::mat4(1.0f), {0.0f, 0.08f, 0.0f})
                    * glm::scale(glm::mat4(1.0f), {0.22f, 0.16f, 0.22f}),
                    {0.045f, 0.055f, 0.075f}, false);
                drawObject(cube, shader.id(), bulbNode.worldTransform()
                    * glm::translate(glm::mat4(1.0f), {0.0f, 0.20f, 0.0f})
                    * glm::scale(glm::mat4(1.0f), {0.25f, 0.10f, 0.25f}),
                    {0.045f, 0.055f, 0.075f}, false);
                const glm::mat4 bulbModel = bulbNode.worldTransform()
                    * glm::scale(glm::mat4(1.0f), {0.23f, 0.27f, 0.23f});
                drawObject(bayterekGlobe, shader.id(), bulbModel, intensity > 0.0f ? glm::vec3(1.0f, 0.70f, 0.28f) : glm::vec3(0.08f),
                    false, intensity > 0.0f, intensity);
            }

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
            if (smokeTest) {
                const std::array<int, 5> frames{{0, 240, 420, 660, 1260}};
                const std::array<const char*, 5> names{{"start.bmp", "block-one.bmp", "skyline-rise.bmp", "fireworks.bmp", "orbit-restored.bmp"}};
                for (std::size_t i = 0; i < frames.size(); ++i) {
                    if (testFrame == frames[i]) {
                        captureBmp(captures / names[i], framebufferWidth, framebufferHeight);
                        std::cout << "PASS " << names[i] << '\n';
                    }
                }
                if (testFrame == 660 && fireworks.vertices().empty())
                    throw std::runtime_error("Finale did not render firework particles");
                if (testFrame == 1260) {
                    if (!director.orbitEnabled()) throw std::runtime_error("Orbit control was not restored");
                    for (float light : blockLights) if (light < 0.99f) throw std::runtime_error("Block was not revealed");
                    keyCallback(window, GLFW_KEY_R, 0, GLFW_REPEAT, 0);
                    if (!director.orbitEnabled()) throw std::runtime_error("Key repeat restarted the reveal");
                    const auto before = director.pose();
                    mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                    cursorPositionCallback(window, inputState().previousMouseX + 20, inputState().previousMouseY + 5);
                    mouseButtonCallback(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
                    scrollCallback(window, 0, 1);
                    if (glm::length(director.pose().position - before.position) < 0.1f)
                        throw std::runtime_error("Mouse orbit/zoom did not change the camera");
                    keyCallback(window, GLFW_KEY_1, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_2, 0, GLFW_PRESS, 0);
                    if (inputState().lights.windows || inputState().lights.lamps) throw std::runtime_error("Lighting toggles failed");
                    keyCallback(window, GLFW_KEY_L, 0, GLFW_PRESS, 0);
                    if (!inputState().lights.windows || !inputState().lights.lamps) throw std::runtime_error("All-light toggle failed");
                    keyCallback(window, GLFW_KEY_G, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_H, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_B, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_K, 0, GLFW_PRESS, 0);
                    if (inputState().showGarlands || inputState().showKhanShatyrLighting || inputState().showBayterekLighting || inputState().showAkOrdaLighting)
                        throw std::runtime_error("Garland/landmark lighting toggles failed");
                    const float brightnessBefore = inputState().landmarkBrightness;
                    keyCallback(window, GLFW_KEY_EQUAL, 0, GLFW_PRESS, 0);
                    if (inputState().landmarkBrightness <= brightnessBefore) throw std::runtime_error("Landmark brightness control failed");
                    keyCallback(window, GLFW_KEY_R, 0, GLFW_PRESS, 0);
                    if (director.orbitEnabled() || !fireworks.vertices().empty()) throw std::runtime_error("Replay did not reset");
                    keyCallback(window, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
                    if (!director.orbitEnabled()) throw std::runtime_error("Skip did not restore orbit");
                }
                if (testFrame == 1261) {
                    captureBmp(captures / "accent-lights-off.bmp", framebufferWidth, framebufferHeight);
                    keyCallback(window, GLFW_KEY_G, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_H, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_B, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_K, 0, GLFW_PRESS, 0);
                    keyCallback(window, GLFW_KEY_L, 0, GLFW_PRESS, 0);
                }
                if (testFrame == 1262) {
                    captureBmp(captures / "main-lights-off.bmp", framebufferWidth, framebufferHeight);
                    keyCallback(window, GLFW_KEY_L, 0, GLFW_PRESS, 0);
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                const auto error = glGetError();
                if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
                ++testFrame;
            }
            glfwSwapBuffers(window);
            glfwPollEvents();

            ++frameCount;
            const double now = glfwGetTime();
            const double elapsed = now - lastFpsUpdate;
            if (elapsed >= 0.5) {
                const double fps = static_cast<double>(frameCount) / elapsed;
                std::ostringstream title;
                title << "Winter Astana | " << director.phaseName() << " | R Replay | Space Skip"
                      << " | 1 Windows " << (inputState().lights.windows ? "ON" : "OFF")
                      << " | 2 Lamps " << (inputState().lights.lamps ? "ON" : "OFF")
                      << " | G Garlands " << (inputState().showGarlands ? "ON" : "OFF")
                      << " | H Khan " << (inputState().showKhanShatyrLighting ? "ON" : "OFF")
                      << " | B Bayterek " << (inputState().showBayterekLighting ? "ON" : "OFF")
                      << " | K AkOrda " << (inputState().showAkOrdaLighting ? "ON" : "OFF")
                      << " | -/+ " << static_cast<int>(inputState().landmarkBrightness*100.0f+0.5f) << "%"
                      << " | L All | Mouse Orbit | Wheel Zoom"
                      << " | FPS: " << std::fixed << std::setprecision(1) << fps;
                glfwSetWindowTitle(window, title.str().c_str());
                frameCount = 0;
                lastFpsUpdate = now;
            }
        }
        glDeleteVertexArrays(1, &snowVao);
        glDeleteBuffers(1, &snowVbo);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = EXIT_FAILURE;
    }

    inputState().director = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}

