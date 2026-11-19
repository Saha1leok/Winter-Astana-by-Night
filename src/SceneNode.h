#pragma once

#include <glm/glm.hpp>

class SceneNode {
public:
    explicit SceneNode(const glm::mat4& localTransform = glm::mat4(1.0f),
                       const SceneNode* parent = nullptr)
        : localTransform_(localTransform), parent_(parent) {}

    glm::mat4 worldTransform() const {
        return parent_ ? parent_->worldTransform() * localTransform_ : localTransform_;
    }

private:
    glm::mat4 localTransform_;
    const SceneNode* parent_;
};
