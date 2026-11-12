#include "Fireworks.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace astana {
void Fireworks::launch(const FireworkCue& cue, double ageSeconds) {
    if (!std::isfinite(ageSeconds) || ageSeconds < 0.0)
        throw std::invalid_argument("Firework age must be finite and non-negative");
    if (ageSeconds >= lifetime) return; // Do not resurrect missed, expired bursts.
    if (bursts_.size() == maxBursts) bursts_.erase(bursts_.begin());
    Burst burst{cue, ageSeconds, {}};
    burst.velocities.reserve(sparksPerBurst);
    std::uint32_t seed = cue.seed;
    auto random = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed >> 8u) / 16777216.0f;
    };
    constexpr float twoPi = 6.28318530718f;
    for (std::size_t i = 0; i < sparksPerBurst; ++i) {
        const float y = 2.0f * random() - 1.0f;
        const float angle = twoPi * random();
        const float radius = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float speed = 2.4f + 2.1f * random();
        burst.velocities.push_back(speed * glm::vec3(radius * std::cos(angle), y, radius * std::sin(angle)));
    }
    bursts_.push_back(std::move(burst));
    rebuildVertices();
}
void Fireworks::update(double deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
        throw std::invalid_argument("Particle delta must be finite and non-negative");
    for (auto& burst : bursts_) burst.age += deltaSeconds;
    bursts_.erase(std::remove_if(bursts_.begin(), bursts_.end(), [](const Burst& b) {
        return b.age >= lifetime;
    }), bursts_.end());
    rebuildVertices();
}
void Fireworks::reset() { bursts_.clear(); vertices_.clear(); }
void Fireworks::rebuildVertices() {
    vertices_.clear();
    vertices_.reserve(maxBursts * sparksPerBurst);
    constexpr float flightSeconds = 0.7f;
    for (const auto& burst : bursts_) {
        const float age = static_cast<float>(burst.age);
        if (age < flightSeconds) {
            const glm::vec3 launch(burst.cue.position.x, 0.6f, burst.cue.position.z);
            const float t = age / flightSeconds;
            const glm::vec3 head = glm::mix(launch, burst.cue.position, t);
            for (int trail = 0; trail < 8; ++trail) {
                const float strength = 1.0f - static_cast<float>(trail) / 8.0f;
                vertices_.push_back({head - glm::vec3(0, trail * 0.16f, 0), burst.cue.color, strength, 4.5f * strength});
            }
            continue;
        }
        const float t = age - flightSeconds;
        const float remaining = std::clamp(1.0f - t / static_cast<float>(lifetime - flightSeconds), 0.0f, 1.0f);
        for (const auto& velocity : burst.velocities) {
            const glm::vec3 position = burst.cue.position + velocity * t + glm::vec3(0, -1.1f * t * t, 0);
            vertices_.push_back({position, burst.cue.color, remaining, 7.0f + 3.0f * remaining});
        }
    }
}
} // namespace astana
