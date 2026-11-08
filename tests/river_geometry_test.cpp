#include "River.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace frozenriver;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
glm::vec3 position(const SurfaceData& mesh, std::size_t i) {
    return {mesh.vertices[i * 6], mesh.vertices[i * 6 + 1], mesh.vertices[i * 6 + 2]};
}
glm::vec3 normal(const SurfaceData& mesh, std::size_t i) {
    return {mesh.vertices[i * 6 + 3], mesh.vertices[i * 6 + 4], mesh.vertices[i * 6 + 5]};
}
void validate(const SurfaceData& mesh, bool horizontal) {
    require(!mesh.indices.empty() && mesh.vertices.size() % 6 == 0 && mesh.indices.size() % 3 == 0, "Invalid buffer layout");
    for (float value : mesh.vertices) require(std::isfinite(value), "Non-finite vertex or normal");
    for (std::size_t i = 0; i < mesh.vertices.size() / 6; ++i) {
        require(std::abs(glm::length(normal(mesh, i)) - 1.0f) < 0.0001f, "Normal is not unit length");
        if (horizontal) require(normal(mesh, i).y > 0.0f, "Surface normal points down");
    }
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const auto a = mesh.indices[i], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
        require(std::max({a, b, c}) < mesh.vertices.size() / 6, "Index outside vertex buffer");
        const glm::vec3 face = glm::cross(position(mesh, b) - position(mesh, a), position(mesh, c) - position(mesh, a));
        require(glm::length(face) > 0.000001f, "Degenerate triangle");
        require(glm::dot(face, normal(mesh, a) + normal(mesh, b) + normal(mesh, c)) > 0, "Winding disagrees with normals");
        if (horizontal) require(face.y > 0, "Triangle culled from above");
    }
}
} // namespace

int main() {
    try {
        const auto ice = createRiver();
        validate(ice, true);
        const auto iceStride = ice.vertices.size() / 6 / (segments + 1);
        float minIce = 1, maxIce = -1;
        for (std::size_t i = 0; i < ice.vertices.size() / 6; ++i) {
            minIce = std::min(minIce, position(ice, i).y);
            maxIce = std::max(maxIce, position(ice, i).y);
        }
        require(maxIce - minIce > 0.002f && maxIce - minIce < 0.01f, "Ice relief should be subtle and non-flat");
        for (bool left : {true, false}) {
            const auto bank = createSnowBank(left), wall = createBankWall(left), drift = createSnowDrift(left);
            validate(bank, true);
            validate(wall, false);
            validate(drift, true);
            const auto bankStride = bank.vertices.size() / 6 / (segments + 1);
            const auto driftStride = drift.vertices.size() / 6 / (segments + 1);
            float narrowest = 10, widest = 0;
            for (int row = 0; row <= segments; ++row) {
                const auto bankEdge = position(bank, row * bankStride + (left ? bankStride - 1 : 0));
                const auto iceEdge = position(ice, row * iceStride + (left ? 0 : iceStride - 1));
                const auto wallTop = position(wall, row * 2), wallBottom = position(wall, row * 2 + 1);
                const auto root = position(drift, row * driftStride), toe = position(drift, row * driftStride + driftStride - 1);
                require(std::abs(bankEdge.x - iceEdge.x) < 0.00001f && std::abs(bankEdge.z - iceEdge.z) < 0.00001f, "Bank and ice shorelines disagree");
                require(glm::length(bankEdge - wallTop) < 0.00001f && glm::length(bankEdge - root) < 0.00001f, "Gap at bank/wall/drift seam");
                require(wallBottom.y < iceEdge.y && bankEdge.y > iceEdge.y, "Wall does not bridge snow to ice");
                require((left ? 1 : -1) * normal(wall, row * 2).x > 0.5f, "Wall faces away from the river");
                require(toe.y > iceHeight(toe.x, toe.z) && toe.y - iceHeight(toe.x, toe.z) < 0.01f, "Drift toe intersects or floats above ice");
                require((left ? 1 : -1) * (toe.x - root.x) > 0 && std::abs(toe.x - riverCenter(toe.z)) < riverHalfWidth(toe.z), "Drift extends away from ice");
                for (std::size_t col = 1; col < driftStride; ++col) {
                    const auto p = position(drift, row * driftStride + col);
                    require(p.y > iceHeight(p.x, p.z), "Snow drift intersects ice");
                }
                const float width = std::abs(root.x - toe.x);
                narrowest = std::min(narrowest, width);
                widest = std::max(widest, width);
            }
            require(narrowest > 0.1f && widest < 1.0f && widest - narrowest > 0.2f, "Snow drift widths lack natural variation");
        }
        std::cout << "All seven meshes: finite data, valid indices, normals, winding, continuous shores, solid ice and raised snow drifts verified.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
