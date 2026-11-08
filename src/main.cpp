#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "River.h"

#include <algorithm>
#include <array>
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

float yawDegrees = -28.0f;
float pitchDegrees = 18.0f;
float cameraDistance = 27.0f;
int cameraMode = 0;
bool showFrost = true;
bool showDrifts = true;
bool dragging = false;
double previousMouseX = 0.0;
double previousMouseY = 0.0;

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_3) cameraMode = key - GLFW_KEY_1;
    if (key == GLFW_KEY_F) showFrost = !showFrost;
    if (key == GLFW_KEY_D) showDrifts = !showDrifts;
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
    dragging = action == GLFW_PRESS;
    if (dragging) {
        glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
    }
}

void cursorPositionCallback(GLFWwindow*, double x, double y) {
    if (!dragging) {
        return;
    }
    const double deltaX = x - previousMouseX;
    const double deltaY = y - previousMouseY;
    previousMouseX = x;
    previousMouseY = y;
    cameraMode = 0;
    yawDegrees += static_cast<float>(deltaX) * 0.25f;
    pitchDegrees = std::clamp(pitchDegrees - static_cast<float>(deltaY) * 0.25f, 8.0f, 80.0f);
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    cameraMode = 0;
    cameraDistance = std::clamp(cameraDistance - static_cast<float>(yOffset) * 1.4f, 10.0f, 48.0f);
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

void drawObject(const Mesh& mesh, unsigned int program, const glm::mat4& model, const glm::vec3& color, bool ice, bool emissive = false, bool snow = false) {
    setMatrix(program, "uModel", model);
    glUniform3fv(glGetUniformLocation(program, "uBaseColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(program, "uIsIce"), ice ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), emissive ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uIsSnow"), snow ? 1 : 0);
    mesh.draw();
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

int main(int argc, char** argv) {
    std::filesystem::path captureDirectory;
    int frameLimit = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--capture-dir" && i + 1 < argc) captureDirectory = argv[++i];
        else if (argument == "--view" && i + 1 < argc) {
            const std::string view = argv[++i];
            if (view == "orbit") cameraMode = 0;
            else if (view == "street") cameraMode = 1;
            else if (view == "above") cameraMode = 2;
            else { std::cerr << "--view requires orbit, street, or above.\n"; return EXIT_FAILURE; }
        }
        else if (argument == "--frames" && i + 1 < argc) {
            try { frameLimit = std::stoi(argv[++i]); } catch (...) { frameLimit = -1; }
            if (frameLimit <= 0) { std::cerr << "--frames requires a positive integer.\n"; return EXIT_FAILURE; }
        } else {
            std::cerr << "Usage: winter_astana [--capture-dir PATH] [--frames COUNT] [--view orbit|street|above]\n";
            return EXIT_FAILURE;
        }
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

        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            const double currentFrameTime = glfwGetTime();
            const float deltaTime = captureDirectory.empty()
                ? static_cast<float>(std::min(currentFrameTime - previousFrameTime, 0.05)) : 0.0f;
            previousFrameTime = currentFrameTime;
            if (!captureDirectory.empty()) {
                cameraMode = std::min(totalFrames, 2);
                showFrost = totalFrames != 3;
                showDrifts = totalFrames != 4;
            }
            const float orbitX = static_cast<float>((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                - (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS));
            const float orbitY = static_cast<float>((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                - (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS));
            if (orbitX != 0 || orbitY != 0) {
                cameraMode = 0;
                yawDegrees += orbitX * 45.0f * deltaTime;
                pitchDegrees = std::clamp(pitchDegrees + orbitY * 35.0f * deltaTime, 8.0f, 80.0f);
            }

            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            const float aspect = framebufferHeight > 0 ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 1.0f;
            const float yaw = glm::radians(yawDegrees);
            const float pitch = glm::radians(pitchDegrees);
            glm::vec3 target(0.5f, 0.9f, -1.5f);
            glm::vec3 cameraPosition = target + cameraDistance * glm::vec3(
                std::cos(pitch) * std::sin(yaw),
                std::sin(pitch),
                std::cos(pitch) * std::cos(yaw));
            if (cameraMode == 1) {
                const float x = riverCenter(9.0f) + riverHalfWidth(9.0f) + 0.75f;
                cameraPosition = {x, snowHeight(x, 9.0f) + 1.60f, 9.0f};
                target = {riverCenter(-2.0f), 0.055f, -2.0f};
            } else if (cameraMode == 2) {
                cameraPosition = {3.0f, 32.0f, 12.0f};
                target = {0.5f, 0.0f, 0.0f};
            }
            const glm::mat4 view = glm::lookAt(cameraPosition, target, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

            glClearColor(0.025f, 0.045f, 0.095f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shader.use();
            setMatrix(shader.id(), "uView", view);
            setMatrix(shader.id(), "uProjection", projection);
            glUniform3fv(glGetUniformLocation(shader.id(), "uCameraPosition"), 1, glm::value_ptr(cameraPosition));
            glUniform1i(glGetUniformLocation(shader.id(), "uShowFrost"), showFrost ? 1 : 0);

            drawObject(leftBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, true);
            drawObject(rightBank, shader.id(), glm::mat4(1.0f), {0.80f, 0.87f, 0.95f}, false, false, true);
            drawObject(river, shader.id(), glm::mat4(1.0f), {0.10f, 0.29f, 0.50f}, true);
            drawObject(leftWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);
            drawObject(rightWall, shader.id(), glm::mat4(1.0f), {0.34f, 0.42f, 0.54f}, false);

            if (showDrifts) {
                drawObject(leftDrift, shader.id(), glm::mat4(1.0f), {0.85f, 0.91f, 0.98f}, false, false, true);
                drawObject(rightDrift, shader.id(), glm::mat4(1.0f), {0.85f, 0.91f, 0.98f}, false, false, true);
            }

            const std::array<glm::vec3, 5> positions{{{-9.2f, 1.75f, -8.5f}, {-6.4f, 2.45f, -8.8f}, {-3.9f, 1.95f, -9.2f}, {6.2f, 2.75f, -8.4f}, {9.0f, 1.55f, -8.8f}}};
            const std::array<glm::vec3, 5> scales{{{2.1f, 2.8f, 1.8f}, {1.8f, 4.2f, 1.7f}, {2.0f, 3.2f, 1.8f}, {2.0f, 4.8f, 2.0f}, {2.8f, 2.4f, 1.8f}}};
            const std::array<glm::vec3, 5> colors{{{0.34f,0.39f,0.52f}, {0.42f,0.48f,0.62f}, {0.38f,0.42f,0.55f}, {0.46f,0.40f,0.55f}, {0.35f,0.45f,0.58f}}};
            for (std::size_t i = 0; i < positions.size(); ++i) {
                const glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i]) * glm::scale(glm::mat4(1.0f), scales[i]);
                drawObject(cube, shader.id(), model, colors[i], false);

                for (int row = 0; row < 3; ++row) {
                    for (int column = -1; column <= 1; column += 2) {
                        const glm::vec3 windowPosition = positions[i] + glm::vec3(
                            static_cast<float>(column) * scales[i].x * 0.24f,
                            -scales[i].y * 0.28f + static_cast<float>(row) * scales[i].y * 0.27f,
                            scales[i].z * 0.505f);
                        const glm::mat4 windowModel = glm::translate(glm::mat4(1.0f), windowPosition)
                            * glm::scale(glm::mat4(1.0f), {0.24f, 0.30f, 0.045f});
                        drawObject(cube, shader.id(), windowModel, {1.0f, 0.62f, 0.16f}, false, true);
                    }
                }
            }

            const std::array<float, 6> lampZ{{-7.0f, -3.5f, 0.0f, 3.5f, 7.0f, 10.0f}};
            for (float z : lampZ) {
                const float x = riverCenter(z) - riverHalfWidth(z) - 0.42f;
                const float groundY = snowHeight(x, z);
                const glm::mat4 postModel = glm::translate(glm::mat4(1.0f), {x, groundY + 0.72f, z})
                    * glm::scale(glm::mat4(1.0f), {0.11f, 1.45f, 0.11f});
                drawObject(cube, shader.id(), postModel, {0.055f, 0.065f, 0.085f}, false);
                const glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), {x, groundY + 1.53f, z})
                    * glm::scale(glm::mat4(1.0f), {0.30f, 0.26f, 0.30f});
                drawObject(cube, shader.id(), lampModel, {1.0f, 0.70f, 0.28f}, false, true);
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

            if (!captureDirectory.empty()) {
                saveFrame(captureDirectory / captureNames[totalFrames], framebufferWidth, framebufferHeight);
            }
            const GLenum error = glGetError();
            if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
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
                title << "Winter Astana | 1 Orbit  2 Street  3 Above | F Frost " << (showFrost ? "on" : "off")
                      << " | D Drifts " << (showDrifts ? "on" : "off") << " | FPS: " << std::fixed << std::setprecision(1) << fps;
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

    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}

