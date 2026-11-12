#include "SceneDirector.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace astana {
namespace {
float ease(double value) {
    const float t = static_cast<float>(std::clamp(value, 0.0, 1.0));
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}
CameraPose lowPose() { return {{1.0f, 5.2f, 25.0f}, {1.0f, 1.3f, -2.0f}}; }
CameraPose finalPose() { return {{-5.5f, 20.0f, 34.0f}, {1.0f, 3.5f, -2.0f}}; }
CameraPose interpolate(const CameraPose& a, const CameraPose& b, float t) {
    return {glm::mix(a.position, b.position, t), glm::mix(a.target, b.target, t)};
}
}

CameraPose OrbitCamera::pose() const {
    const float yaw = glm::radians(yaw_), pitch = glm::radians(pitch_);
    return {target_ + distance_ * glm::vec3(std::cos(pitch) * std::sin(yaw),
        std::sin(pitch), std::cos(pitch) * std::cos(yaw)), target_};
}
void OrbitCamera::adopt(const CameraPose& pose) {
    const glm::vec3 offset = pose.position - pose.target;
    distance_ = glm::length(offset);
    if (distance_ < 0.001f) throw std::invalid_argument("Camera position equals target");
    target_ = pose.target;
    yaw_ = glm::degrees(std::atan2(offset.x, offset.z));
    pitch_ = glm::degrees(std::asin(std::clamp(offset.y / distance_, -1.0f, 1.0f)));
}
void OrbitCamera::drag(float deltaX, float deltaY) {
    yaw_ = std::remainder(yaw_ + deltaX * 0.25f, 360.0f);
    pitch_ = std::clamp(pitch_ - deltaY * 0.25f, 8.0f, 80.0f);
}
void OrbitCamera::zoom(float wheelDelta) {
    distance_ = std::clamp(distance_ - wheelDelta * 1.4f, 10.0f, 48.0f);
}

SceneDirector::SceneDirector(RevealTimeline timeline, RevealHooks hooks)
    : timeline_(std::move(timeline)), hooks_(std::move(hooks)) {
    auto positive = [](double x) { return std::isfinite(x) && x > 0.0; };
    if (!positive(timeline_.approachSeconds) || !positive(timeline_.riseSeconds)
        || !positive(timeline_.finaleSeconds) || !positive(timeline_.lightFadeSeconds))
        throw std::invalid_argument("Reveal durations must be finite and positive");
    double previous = -1.0;
    for (double progress : timeline_.blockProgress) {
        if (!std::isfinite(progress) || progress < 0.0 || progress > 1.0 || progress <= previous)
            throw std::invalid_argument("Block progress must increase within [0,1]");
        previous = progress;
    }
    previous = -1.0;
    for (const auto& cue : timeline_.fireworks) {
        if (!std::isfinite(cue.delaySeconds) || cue.delaySeconds < 0.0
            || cue.delaySeconds < previous || cue.delaySeconds >= timeline_.finaleSeconds)
            throw std::invalid_argument("Firework cues must be ordered within the finale");
        previous = cue.delaySeconds;
    }
}
double SceneDirector::riseEnd() const { return timeline_.approachSeconds + timeline_.riseSeconds; }
void SceneDirector::setIntensity(std::size_t index, float intensity) {
    lights_[index] = intensity;
    if (hooks_.setBlockLight) hooks_.setBlockLight(static_cast<int>(index) + 1, intensity);
}
void SceneDirector::startReveal() {
    startPose_ = pose(); // Replays start from the current view without a cut.
    phase_ = Phase::Approach;
    elapsed_ = 0.0;
    nextBlock_ = nextFirework_ = 0;
    lightStarts_.fill(-1.0);
    wasVisible_.fill(false);
    for (std::size_t i = 0; i < lights_.size(); ++i) setIntensity(i, 0.0f);
    if (hooks_.resetFireworks) hooks_.resetFireworks();
}
void SceneDirector::restoreOrbit() {
    orbit_.adopt(pose()); // Preserve position AND target at the handoff.
    phase_ = Phase::Orbit;
    if (hooks_.orbitRestored) hooks_.orbitRestored();
}
void SceneDirector::skipReveal() {
    if (orbitEnabled()) return;
    // Keep the current cinematic pose when skipping, including during approach.
    restoreOrbit();
    nextBlock_ = lights_.size();
    nextFirework_ = timeline_.fireworks.size();
    lightStarts_.fill(-1.0);
    for (std::size_t i = 0; i < lights_.size(); ++i) setIntensity(i, 1.0f);
    if (hooks_.resetFireworks) hooks_.resetFireworks();
}
void SceneDirector::update(double deltaSeconds, const std::array<bool, 4>& visibleBlocks) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
        throw std::invalid_argument("Frame delta must be finite and non-negative");
    elapsed_ += deltaSeconds;
    if (!orbitEnabled()) {
        if (elapsed_ < timeline_.approachSeconds) phase_ = Phase::Approach;
        else if (elapsed_ < riseEnd()) phase_ = Phase::Rise;
        else phase_ = Phase::Finale;
    }
    while (nextBlock_ < lights_.size()) {
        const std::size_t i = nextBlock_;
        double due = timeline_.approachSeconds + timeline_.riseSeconds * timeline_.blockProgress[i];
        // Preserve a visible gap if an earlier block was delayed off screen.
        if (i > 0) due = std::max(due, lightStarts_[i - 1] + timeline_.lightFadeSeconds);
        if (elapsed_ < due || !visibleBlocks[i]) break;
        lightStarts_[i] = wasVisible_[i] ? due : elapsed_;
        ++nextBlock_;
    }
    wasVisible_ = visibleBlocks;
    for (std::size_t i = 0; i < lights_.size(); ++i) {
        if (lightStarts_[i] >= 0.0 && lights_[i] < 1.0f)
            setIntensity(i, ease((elapsed_ - lightStarts_[i]) / timeline_.lightFadeSeconds));
    }
    while (!orbitEnabled() && nextFirework_ < timeline_.fireworks.size()) {
        const auto& cue = timeline_.fireworks[nextFirework_];
        const double due = riseEnd() + cue.delaySeconds;
        if (elapsed_ < due) break;
        if (hooks_.launchFirework) hooks_.launchFirework(cue, elapsed_ - due);
        ++nextFirework_;
    }
    if (!orbitEnabled() && elapsed_ >= riseEnd() + timeline_.finaleSeconds) restoreOrbit();
}
CameraPose SceneDirector::pose() const {
    if (orbitEnabled()) return orbit_.pose();
    if (elapsed_ < timeline_.approachSeconds)
        return interpolate(startPose_, lowPose(), ease(elapsed_ / timeline_.approachSeconds));
    return interpolate(lowPose(), finalPose(), ease((elapsed_ - timeline_.approachSeconds) / timeline_.riseSeconds));
}
void SceneDirector::drag(float deltaX, float deltaY) {
    if (orbitEnabled()) orbit_.drag(deltaX, deltaY);
}
void SceneDirector::zoom(float wheelDelta) {
    if (orbitEnabled()) orbit_.zoom(wheelDelta);
}
float SceneDirector::blockIntensity(int blockId) const {
    if (blockId < 1 || blockId > 4) throw std::out_of_range("City block ID must be 1..4");
    return lights_[static_cast<std::size_t>(blockId - 1)];
}
const char* SceneDirector::phaseName() const {
    switch (phase_) {
    case Phase::Orbit: return "Orbit";
    case Phase::Approach: return "Approach";
    case Phase::Rise: return "Skyline rise";
    case Phase::Finale: return "Fireworks finale";
    }
    return "Orbit";
}
bool skylineVisible(const glm::mat4& viewProjection, const glm::vec3& probe) {
    const glm::vec4 clip = viewProjection * glm::vec4(probe, 1.0f);
    return clip.w > 0.0f && std::abs(clip.x) <= clip.w * 0.95f
        && std::abs(clip.y) <= clip.w * 0.95f && std::abs(clip.z) <= clip.w;
}
} // namespace astana
