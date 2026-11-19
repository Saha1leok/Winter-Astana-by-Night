#include "SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

namespace {

bool nearlyEqual(float actual, float expected) {
    return std::abs(actual - expected) < 0.0001F;
}

}  // namespace

int main() {
    SceneNode lampRoot(glm::translate(glm::mat4(1.0F), glm::vec3(2.0F, 0.0F, -3.0F)));
    SceneNode bulb(glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 4.5F, 0.0F)), &lampRoot);

    const glm::vec4 worldPosition = bulb.worldTransform() * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    if (!nearlyEqual(worldPosition.x, 2.0F) || !nearlyEqual(worldPosition.y, 4.5F) ||
        !nearlyEqual(worldPosition.z, -3.0F) || !nearlyEqual(worldPosition.w, 1.0F)) {
        std::cerr << "Parent-child world transform is incorrect\n";
        return 1;
    }

    std::cout << "Scene hierarchy test passed\n";
    return 0;
}
