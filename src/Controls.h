#pragma once

#include "Lighting.h"

#include <GLFW/glfw3.h>

namespace astana {

class SceneDirector;

struct InputState {
    SceneDirector* director = nullptr;
    lighting::State lights;
    bool showGarlands = true;
    bool showKhanShatyrLighting = true;
    bool showBayterekLighting = true;
    bool showAkOrdaLighting = true;
    float landmarkBrightness = 0.65f;
    bool dragging = false;
    double previousMouseX = 0.0;
    double previousMouseY = 0.0;
};

InputState& inputState();
void keyCallback(GLFWwindow*, int key, int, int action, int);
void framebufferSizeCallback(GLFWwindow*, int width, int height);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int);
void cursorPositionCallback(GLFWwindow*, double x, double y);
void scrollCallback(GLFWwindow*, double, double yOffset);

}
