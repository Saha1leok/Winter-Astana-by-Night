#include <GL/glew.h>

#include "Controls.h"

#include "SceneDirector.h"

#include <algorithm>

namespace astana {

InputState& inputState() {
    static InputState state;
    return state;
}

void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto& state = inputState();
    if (key == GLFW_KEY_1) state.lights.windows = !state.lights.windows;
    if (key == GLFW_KEY_2) state.lights.lamps = !state.lights.lamps;
    if (key == GLFW_KEY_L) state.lights.toggleAll();
    if (key == GLFW_KEY_G) state.showGarlands = !state.showGarlands;
    if (key == GLFW_KEY_H) state.showKhanShatyrLighting = !state.showKhanShatyrLighting;
    if (key == GLFW_KEY_B) state.showBayterekLighting = !state.showBayterekLighting;
    if (key == GLFW_KEY_K) state.showAkOrdaLighting = !state.showAkOrdaLighting;
    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
        state.landmarkBrightness = std::clamp(state.landmarkBrightness - 0.10f, 0.20f, 1.0f);
    if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
        state.landmarkBrightness = std::clamp(state.landmarkBrightness + 0.10f, 0.20f, 1.0f);
    if (!state.director) return;
    if (key == GLFW_KEY_R || key == GLFW_KEY_SPACE) {
        state.dragging = false;
        if (key == GLFW_KEY_R) state.director->startReveal();
        else state.director->skipReveal();
    }
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    auto& state = inputState();
    state.dragging = action == GLFW_PRESS && state.director && state.director->orbitEnabled();
    if (state.dragging)
        glfwGetCursorPos(window, &state.previousMouseX, &state.previousMouseY);
}

void cursorPositionCallback(GLFWwindow*, double x, double y) {
    auto& state = inputState();
    if (!state.dragging || !state.director || !state.director->orbitEnabled()) return;
    const double deltaX = x - state.previousMouseX;
    const double deltaY = y - state.previousMouseY;
    state.previousMouseX = x;
    state.previousMouseY = y;
    state.director->drag(static_cast<float>(deltaX), static_cast<float>(deltaY));
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (inputState().director)
        inputState().director->zoom(static_cast<float>(yOffset));
}

}
