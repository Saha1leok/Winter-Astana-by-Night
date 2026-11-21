#include "RenderUtils.h"

#include "Mesh.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

namespace astana {

void setMatrix(unsigned int program, const char* name, const glm::mat4& matrix) {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void drawObject(const Mesh& mesh, unsigned int program, const glm::mat4& model,
                const glm::vec3& color, bool ice, bool emissive,
                float emissionStrength, bool ground) {
    setMatrix(program, "uModel", model);
    glUniform3fv(glGetUniformLocation(program, "uBaseColor"), 1, glm::value_ptr(color));
    glUniform1i(glGetUniformLocation(program, "uIsIce"), ice ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uEmissive"), emissive ? 1 : 0);
    glUniform1i(glGetUniformLocation(program, "uIsGround"), ground ? 1 : 0);
    glUniform1f(glGetUniformLocation(program, "uEmissionStrength"), emissionStrength);
    mesh.draw();
}

}
