# Camera reveal and team integration

Base: Tolganay's `feature/city-layout-modeling` at `3eabe9396d0a010b144bfa816f20ccdade4b310a`.
The building layout, monuments, roads, terrain, river, trees and snow remain in place. The old always-on window placeholders now use Member 2's generated windows. The original promenade and bridge lamp locations remain, with eight additional block anchors connected to the lighting system.

## Member 2: actual lighting integration

`src/Lighting.h`, `src/Lighting.cpp` and the capture helper `src/FrameCapture.h` are copied unchanged from Dilnaz's commit `dc89196d60354dd997e3f3c39a655102edd2c3a4` on `dilnaz-lighting`. The local diffuse-light section in `shaders/scene.frag` comes from the same commit and is extended with fade weights. The older scene in that branch is not substituted for Tolganay's newer city.

The adapter in `main.cpp` associates `lighting::Window` and `lighting::Lamp` with the existing stable city block IDs **1–4**. Windows are generated once for every building. `RevealHooks::setBlockLight(int blockId, float intensity)` updates each block's fade in `[0,1]`. Manual `lighting::State` toggles multiply this reveal state, so a reveal never overrides a user's disabled windows/lamps.

`lighting::uploadLamps` retains its existing API and 12-light limit. Eight block-anchor lamps and four bridge lamps contribute local diffuse light. All remaining original bulbs still emit and follow their assigned block. `uLampStrengths[]` uses the same compact ordering as `uLampPositions[]`. Emissive geometry uses `uEmissionStrength` (set for every draw).

## Camera behavior

`SceneDirector` owns camera state; callbacks route input through it. A quintic easing curve smoothly approaches the low viewpoint, rises to a wide skyline view and holds that view for the finale. Orbit/zoom input is ignored during cinematic motion. At the handoff, `OrbitCamera::adopt` reconstructs yaw, pitch, distance and target from the final pose, so there is no camera cut. Replaying starts from the current pose; skipping adopts the current cinematic pose, clears particles and completes the city lights.

Each block has a skyline probe at its tallest modeled building. A light cue waits until its probe enters the view frustum. A delayed block preserves the order and at least one fade-duration gap before the next block. In a narrow window an off-screen group can remain pending until the user orbits it into view; camera control is still returned on time. Probe visibility does not test occlusion by other buildings.

## Member 5: timing interface

Pass a customized `astana::RevealTimeline` to `SceneDirector(timeline, hooks)`. There is no GLFW clock inside the director; call `update(deltaSeconds, visibleBlocks)` once per frame. `blockProgress` must strictly increase within `[0,1]`, durations must be positive, and fireworks must be ordered within the finale. Invalid schedules throw descriptive exceptions.

| Cue | Default |
| --- | --- |
| Approach | 0–1.5 s |
| Camera rise | 1.5–9.5 s |
| Blocks 1 / 2 / 3 / 4 | 2.94 / 4.38 / 5.82 / 7.26 s, subject to visibility |
| Block fade | 0.65 s |
| Firework launch requests | 9.75 / 10.45 / 11.15 / 11.90 / 12.15 s |
| Return orbit control | 14.5 s |

`FireworkCue::delaySeconds` is relative to the end of the rise. Events fire exactly once, including when a slow frame crosses several cues. The callback receives an age offset for late dispatch. `startReveal()` resets event cursors and particles; `skipReveal()` suppresses remaining cues. No separate Member 5 implementation was present in the remote branches inspected for this change.

## Member 3: particle interface

Connect the team's particle module to these hooks:

```cpp
astana::RevealHooks hooks;
hooks.launchFirework = [&](const astana::FireworkCue& cue, double ageSeconds) {
    particles.launch(cue, ageSeconds);
};
hooks.resetFireworks = [&] { particles.reset(); };
```

The cue carries a world-space explosion position, RGB color and reproducible seed. The fallback `astana::Fireworks` supplies a 0.7 s rocket ascent, radial colored sparks, analytic gravity, fading and cleanup after 3.5 s. It is independent of OpenGL; `FireworkRenderer` uploads its vertices and draws additive point sprites with depth testing and depth writes disabled, then restores the render state used by this scene. Particle storage is bounded to 24 bursts of 96 sparks. Update existing particles before updating the director, so newly dispatched aged cues are not advanced twice.

No separate Member 3 firework module was present in the inspected remote branches (`main`, `feature/city-layout-modeling`, `dilnaz-lighting`, `codex/frozen-river-banks`). The fallback makes the scene work now, but integration with a later Member 3 implementation still needs its actual API. Fireworks currently have no bloom, sound, shadows or dynamic illumination of the city.

## Verification

- C++17 build with `-Wall -Wextra -Wpedantic` on AppleClang 17.
- CTest logic coverage: seamless handoff, input lock/limits, replay/skip, four-block order/fades, deferred visibility, exactly-once cues, 30/60/144 FPS and stalled frames, timeline validation, deterministic motion, particle capacity and expiry.
- OpenGL smoke test: all shaders compile/link, five scene captures, no OpenGL errors, all blocks finish, particles render, orbit/zoom and lighting controls work, replay and skip work.
- Windows/Linux rendering has not been exercised in this environment.
