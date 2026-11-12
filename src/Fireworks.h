#pragma once

#include "SceneDirector.h"
#include <cstddef>
#include <vector>

namespace astana {
struct SparkVertex {
    glm::vec3 position;
    glm::vec3 color;
    float alpha;
    float size;
};

// Replaceable Member 3 implementation; contains no OpenGL or timing decisions.
// Analytic motion gives the same positions at a given age at every frame rate.
class Fireworks {
public:
    static constexpr std::size_t sparksPerBurst = 96;
    static constexpr std::size_t maxBursts = 24;
    static constexpr double lifetime = 3.5;

    void launch(const FireworkCue& cue, double ageSeconds = 0.0);
    void update(double deltaSeconds);
    void reset();
    const std::vector<SparkVertex>& vertices() const { return vertices_; }

private:
    struct Burst {
        FireworkCue cue;
        double age;
        std::vector<glm::vec3> velocities;
    };
    void rebuildVertices();
    std::vector<Burst> bursts_;
    std::vector<SparkVertex> vertices_;
};
} // namespace astana
