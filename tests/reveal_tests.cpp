#include "SceneDirector.h"
#include "Fireworks.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
const std::array<bool, 4> visible{{true, true, true, true}};
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void samePose(const astana::CameraPose& a, const astana::CameraPose& b, const char* message) {
    require(glm::length(a.position - b.position) < 0.0001f && glm::length(a.target - b.target) < 0.0001f, message);
}
template <typename F> void mustThrow(F action, const char* message) {
    bool threw = false;
    try { action(); } catch (const std::exception&) { threw = true; }
    require(threw, message);
}

void orbitAndHandoff() {
    astana::OrbitCamera orbit;
    orbit.drag(1.0e6f, -1.0e6f);
    orbit.zoom(-1.0e6f);
    auto pose = orbit.pose();
    require(std::abs(glm::length(pose.position - pose.target) - 48.0f) < 0.001f, "Zoom-out bound");
    require(std::isfinite(pose.position.x), "Yaw must remain finite");
    orbit.zoom(1.0e6f);
    require(std::abs(glm::length(orbit.pose().position - orbit.pose().target) - 10.0f) < 0.001f, "Zoom-in bound");

    int handoffs = 0;
    astana::RevealHooks hooks;
    hooks.orbitRestored = [&] { ++handoffs; };
    astana::SceneDirector director({}, hooks);
    const auto initial = director.pose();
    director.startReveal();
    samePose(initial, director.pose(), "Reveal start must not cut");
    director.update(1.5, visible);
    require(director.phase() == astana::SceneDirector::Phase::Rise, "Approach boundary");
    const auto low = director.pose();
    director.drag(100, 100);
    director.zoom(10);
    samePose(low, director.pose(), "Input must not mutate a cinematic camera");
    director.update(8, visible);
    require(director.phase() == astana::SceneDirector::Phase::Finale, "Rise boundary");
    const auto high = director.pose();
    require(high.position.y > low.position.y + 10, "Camera must rise");
    director.update(5, visible);
    require(director.orbitEnabled() && handoffs == 1, "Finale must hand back orbit exactly once");
    samePose(high, director.pose(), "Handoff must preserve view");
    director.update(10, visible);
    require(handoffs == 1, "Repeated handoff");
    director.drag(40, 10);
    director.zoom(1);
    require(glm::length(high.position - director.pose().position) > 0.1f, "Orbit remains interactive");
    const auto moved = director.pose();
    director.startReveal();
    samePose(moved, director.pose(), "Replay must start from the current view");
    director.update(0.4, visible);
    const auto beforeSkip = director.pose();
    director.skipReveal();
    samePose(beforeSkip, director.pose(), "Skip must preserve view");
    require(director.orbitEnabled(), "Skip restores input");
    for (int i = 1; i <= 4; ++i) require(director.blockIntensity(i) == 1, "Skip lights the city");
}

void cuesAndFrameRates() {
    astana::CameraPose reference{};
    for (int rate : {30, 60, 144}) {
        std::vector<int> order;
        std::array<float, 4> last{};
        std::vector<std::uint32_t> seeds;
        int resets = 0;
        astana::RevealHooks hooks;
        hooks.setBlockLight = [&](int id, float value) {
            const auto index = static_cast<std::size_t>(id - 1);
            require(value >= last[index] || value == 0, "Light fade must be monotonic");
            if (value > 0 && last[index] == 0) order.push_back(id);
            last[index] = value;
        };
        hooks.launchFirework = [&](const auto& cue, double age) {
            require(age >= 0 && age <= 1.0 / rate + 0.0001, "Cue crossed at wrong frame");
            seeds.push_back(cue.seed);
        };
        hooks.resetFireworks = [&] { ++resets; };
        astana::SceneDirector director({}, hooks);
        director.startReveal();
        director.update(0, visible);
        for (int i = 0; i < rate * 16; ++i) director.update(1.0 / rate, visible);
        require(order == std::vector<int>({1, 2, 3, 4}), "Blocks must light once in order");
        require(seeds == std::vector<std::uint32_t>({31, 71, 97, 123, 157}), "Fireworks must dispatch once in order");
        for (float intensity : last) require(intensity == 1, "All blocks must finish fading");
        require(director.orbitEnabled(), "Frame-rate independent completion");
        if (rate == 30) reference = director.pose();
        else samePose(reference, director.pose(), "Frame-rate independent final view");
        director.startReveal();
        require(resets == 2 && director.blockIntensity(1) == 0, "Replay resets lights and particles");
    }
    std::vector<double> ages;
    astana::RevealHooks hooks;
    hooks.launchFirework = [&](const auto&, double age) { ages.push_back(age); };
    astana::SceneDirector stalled({}, hooks);
    stalled.startReveal();
    stalled.update(13, visible);
    require(ages.size() == 5 && std::abs(ages.back() - 0.85) < 0.00001, "A stalled frame must not drop or mistime cues");
    stalled.update(10, visible);
    require(ages.size() == 5, "Stalled frame duplicated fireworks");
}

void visibilityAndValidation() {
    astana::SceneDirector director;
    director.startReveal();
    auto hidden = visible;
    hidden[0] = false;
    director.update(16, hidden);
    require(director.orbitEnabled(), "Hidden skyline must not trap camera input");
    for (int id = 1; id <= 4; ++id) require(director.blockIntensity(id) == 0, "Hidden first block must defer ordered lights");
    director.update(0, visible);
    require(director.blockIntensity(1) == 0 && director.blockIntensity(2) == 0, "Deferred blocks must fade separately");
    for (int i = 0; i < 400; ++i) director.update(0.01, visible);
    for (int id = 1; id <= 4; ++id) require(director.blockIntensity(id) == 1, "Deferred lights must catch up during orbit");
    const auto projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    require(astana::skylineVisible(projection, {0, 0, -10}), "In-frustum probe");
    require(!astana::skylineVisible(projection, {0, 0, 10}), "Behind-camera probe");
    require(!astana::skylineVisible(projection, {100, 0, -10}), "Off-screen probe");
    mustThrow([&] { director.update(-1, visible); }, "Negative delta");
    mustThrow([&] { director.update(std::numeric_limits<double>::quiet_NaN(), visible); }, "NaN delta");
    mustThrow([&] { director.blockIntensity(0); }, "Invalid block ID");
    astana::RevealTimeline invalid;
    invalid.riseSeconds = 0;
    mustThrow([&] { astana::SceneDirector bad(invalid); }, "Invalid duration");
    invalid = {};
    invalid.blockProgress[2] = 0.1;
    mustThrow([&] { astana::SceneDirector bad(invalid); }, "Unordered blocks");
    invalid = {};
    invalid.fireworks.front().delaySeconds = 99;
    mustThrow([&] { astana::SceneDirector bad(invalid); }, "Invalid firework timing");
}

void particles() {
    const astana::FireworkCue cue{0, {1, 12, -8}, {1, 0.7f, 0.2f}, 42};
    astana::Fireworks fine, coarse;
    fine.launch(cue);
    require(!fine.vertices().empty(), "Rocket trail must be visible");
    for (int i = 0; i < 120; ++i) fine.update(1.0 / 120);
    coarse.launch(cue, 1.0);
    require(fine.vertices().size() == astana::Fireworks::sparksPerBurst, "Burst particle count");
    for (std::size_t i = 0; i < fine.vertices().size(); ++i)
        require(glm::length(fine.vertices()[i].position - coarse.vertices()[i].position) < 0.0001f, "Particle motion must be independent of frame rate");
    for (int i = 0; i < 100; ++i) coarse.launch(cue, 1);
    require(coarse.vertices().size() <= astana::Fireworks::maxBursts * astana::Fireworks::sparksPerBurst, "Bounded GPU particle upload");
    coarse.update(10);
    require(coarse.vertices().empty(), "Expired particles must be removed");
    coarse.launch(cue, 20);
    require(coarse.vertices().empty(), "Do not resurrect expired cues");
    fine.reset();
    require(fine.vertices().empty(), "Replay must clear particles");
    mustThrow([&] { fine.launch(cue, -1); }, "Invalid particle age");
}
}

int main() {
    try {
        orbitAndHandoff(); std::cout << "PASS orbit limits, replay, skip, seamless handoff\n";
        cuesAndFrameRates(); std::cout << "PASS block order, fades, cues at 30/60/144 FPS and stalled frames\n";
        visibilityAndValidation(); std::cout << "PASS skyline visibility and timeline validation\n";
        particles(); std::cout << "PASS deterministic particles, expiration and capacity\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
