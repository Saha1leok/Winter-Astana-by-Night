#pragma once

#include "Fireworks.h"
#include "Shader.h"
#include <filesystem>

namespace astana {
class FireworkRenderer {
public:
    explicit FireworkRenderer(const std::filesystem::path& shaderPath);
    ~FireworkRenderer();
    FireworkRenderer(const FireworkRenderer&) = delete;
    FireworkRenderer& operator=(const FireworkRenderer&) = delete;
    void draw(const Fireworks& fireworks, const glm::mat4& view, const glm::mat4& projection) const;

private:
    Shader shader_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
};
} // namespace astana
