#include "FireworkRenderer.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstddef>

namespace astana {
FireworkRenderer::FireworkRenderer(const std::filesystem::path& shaderPath)
    : shader_((shaderPath / "fireworks.vert").string(), (shaderPath / "fireworks.frag").string()) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(Fireworks::maxBursts * Fireworks::sparksPerBurst * sizeof(SparkVertex)), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SparkVertex), reinterpret_cast<void*>(offsetof(SparkVertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SparkVertex), reinterpret_cast<void*>(offsetof(SparkVertex, color)));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SparkVertex), reinterpret_cast<void*>(offsetof(SparkVertex, alpha)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(SparkVertex), reinterpret_cast<void*>(offsetof(SparkVertex, size)));
    for (unsigned int i = 0; i < 4; ++i) glEnableVertexAttribArray(i);
    glBindVertexArray(0);
}
FireworkRenderer::~FireworkRenderer() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
}
void FireworkRenderer::draw(const Fireworks& fireworks, const glm::mat4& view, const glm::mat4& projection) const {
    const auto& vertices = fireworks.vertices();
    if (vertices.empty()) return;
    shader_.use();
    glUniformMatrix4fv(glGetUniformLocation(shader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size() * sizeof(SparkVertex)), vertices.data());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size()));
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
} // namespace astana
