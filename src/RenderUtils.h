#pragma once

#include <glm/glm.hpp>

namespace astana {

class Mesh;

void setMatrix(unsigned int program, const char* name, const glm::mat4& matrix);
void drawObject(const Mesh& mesh, unsigned int program, const glm::mat4& model,
                const glm::vec3& color, bool ice, bool emissive = false,
                float emissionStrength = 1.0f, bool ground = false);

}
