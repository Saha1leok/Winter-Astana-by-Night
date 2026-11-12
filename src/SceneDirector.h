#pragma once

#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace astana {
struct CameraPose {
    glm::vec3 position;
    glm::vec3 target;
};

class OrbitCamera {
public:
    CameraPose pose() const;
    void adopt(const CameraPose& pose);
    void drag(float deltaX, float deltaY);
    void zoom(float wheelDelta);

private:
    glm::vec3 target_{2.0f, 1.55f, 0.4f};
    float yaw_ = -34.0f;
    float pitch_ = 16.0f;
    float distance_ = 27.0f;
};

// Member 3 consumes these world-space burst requests. delaySeconds is relative
// to the end of the camera rise; the particle implementation owns flight time.
struct FireworkCue {
    double delaySeconds;
    glm::vec3 position;
    glm::vec3 color;
    std::uint32_t seed;
};

// Member 5 owns these timings, independently of camera input and rendering.
struct RevealTimeline {
    double approachSeconds = 1.5;
    double riseSeconds = 8.0;
    double finaleSeconds = 5.0;
    double lightFadeSeconds = 0.65;
    std::array<double, 4> blockProgress{{0.18, 0.36, 0.54, 0.72}};
    std::vector<FireworkCue> fireworks{
        {0.25, {-8.0f, 11.0f, -7.0f}, {1.0f, 0.48f, 0.12f}, 31},
        {0.95, { 1.0f, 13.0f, -9.0f}, {0.18f, 0.70f, 1.0f}, 71},
        {1.65, {10.0f, 11.5f, -8.0f}, {1.0f, 0.25f, 0.52f}, 97},
        {2.40, {-4.0f, 12.5f, -8.0f}, {0.65f, 1.0f, 0.35f}, 123},
        {2.65, { 7.0f, 13.0f, -9.0f}, {1.0f, 0.80f, 0.25f}, 157}
    };
};

struct RevealHooks {
    // Member 2: stable city block IDs 1..4, intensity in [0,1].
    std::function<void(int, float)> setBlockLight;
    // Age makes cue dispatch correct even when one frame crosses several cues.
    std::function<void(const FireworkCue&, double ageSeconds)> launchFirework;
    std::function<void()> resetFireworks;
    std::function<void()> orbitRestored;
};

class SceneDirector {
public:
    enum class Phase { Orbit, Approach, Rise, Finale };

    explicit SceneDirector(RevealTimeline timeline = {}, RevealHooks hooks = {});
    void startReveal();
    void skipReveal();
    void update(double deltaSeconds, const std::array<bool, 4>& visibleBlocks);
    void drag(float deltaX, float deltaY);
    void zoom(float wheelDelta);
    CameraPose pose() const;
    bool orbitEnabled() const { return phase_ == Phase::Orbit; }
    Phase phase() const { return phase_; }
    double elapsed() const { return elapsed_; }
    float blockIntensity(int blockId) const;
    const char* phaseName() const;

private:
    double riseEnd() const;
    void restoreOrbit();
    void setIntensity(std::size_t index, float intensity);

    RevealTimeline timeline_;
    RevealHooks hooks_;
    OrbitCamera orbit_;
    Phase phase_ = Phase::Orbit;
    CameraPose startPose_{};
    double elapsed_ = 0.0;
    std::array<float, 4> lights_{{1, 1, 1, 1}};
    std::array<double, 4> lightStarts_{{-1, -1, -1, -1}};
    std::array<bool, 4> wasVisible_{{false, false, false, false}};
    std::size_t nextBlock_ = 4;
    std::size_t nextFirework_ = 0;
};

// Skyline probes are supplied by the city layout; this tests the view frustum,
// not building occlusion. A hidden block waits until its probe enters the view.
bool skylineVisible(const glm::mat4& viewProjection, const glm::vec3& probe);
} // namespace astana
