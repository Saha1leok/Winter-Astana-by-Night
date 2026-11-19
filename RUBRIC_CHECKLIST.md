# Final Project rubric checklist

This checklist records verifiable repository evidence. It does not replace the report, presentation, demo video, oral explanation, TA-machine FPS test, or team contribution history.

## Mandatory requirements R1-R8

| Requirement | Current evidence | Status |
| --- | --- | --- |
| R1 Modern pipeline | OpenGL 3.3 Core; VAO/VBO/EBO in `src/main.cpp`; separate GLSL files; no fixed-function rendering calls | Verified in source |
| R2 Scene setup | Snow banks, river, walls, buildings, landmarks, lamps, windows, trees, snow and fireworks | Verified in source |
| R3 Transformations | GLM Model/View/Projection; `SceneNode` lamp root with bulb child inheriting the parent transform; `scene_hierarchy` test | Verified in source/test |
| R4 Lighting | Per-fragment Blinn-Phong ambient/diffuse/specular directional light plus toggleable attenuated lamp lights | Verified in shader/source |
| R5 Equivalent texture effect | World-space procedural ice cracks/shimmer in `scene.frag` | Verified in shader |
| R6 Interaction | Mouse orbit, wheel zoom, replay/skip, window/lamp/all-light toggles | Verified in source and smoke test |
| R7 Performance | FPS shown in the window title; local FPS must be measured again after the final build; TA-machine result is unknown | Partly verified |
| R8 Build | CMake build steps are in README; build and logic tests pass on the preparation Mac | Verified locally only |

## Rubric risks before submission

- D2 is still partial: `src/main.cpp` is over 500 lines and should be split into Mesh, Terrain, CityScene, and Application modules by the team.
- D3 needs confirmed team names and truthful roles in README. Do not invent contributions.
- D4 depends on the real Git history: every member needs meaningful commits over time.
- E1-E7 require the instructor's report template, 15+ credible Q1-Q2 journal sources, real screenshots, an actual hardware/FPS table, references, credits, and a specific AI Use Note.
- F1-F4 require each member to explain their own part, shader/math, and project-wide questions without reading.
- G1-G5 require a real 3-6 minute 1080p capture with the FPS counter visible and narration/captions covering R1-R7 and claimed stretch goals.
- Create tag `v1.0` only after the final repository, report, slides, and video are genuinely ready.

## Suggested evidence order for the demo video

1. Start at 1280x720 with the FPS title visible.
2. Show the cinematic reveal and at least five distinct scene objects.
3. Orbit with the mouse and zoom with the wheel.
4. Toggle windows (`1`), lamps (`2`), and all lights (`L`).
5. Replay (`R`) and skip (`Space`) the reveal.
6. Point out the lamp parent-child hierarchy and the Blinn-Phong shader terms.
7. Show procedural ice and the fireworks/snow stretch goals.
