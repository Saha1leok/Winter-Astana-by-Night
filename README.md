# Topic 7: Winter Astana by Night

Week 7 starter project for Computer Graphics Fundamentals. It uses C++17 and OpenGL 3.3 Core without a game engine.

## Current progress

- two snowy banks with terrain mounds, correctly oriented normals, and fine procedural snow grain;
- a curved frozen river with subtle solid-surface relief, layered cracks, irregular frost patches, and view-dependent ice highlights;
- raised snow drifts with varying widths along both shorelines;
- simple raised embankment walls along the river;
- five optional low-poly buildings for scale and composition;
- animated falling snow particles;
- warm glowing windows and simple lamps along the embankment;
- distance fog and static procedural ice texturing;
- mouse/keyboard orbit camera plus street-level and aerial inspection views;
- custom GLSL vertex and fragment shaders;
- VAO, VBO and EBO mesh data;
- Model, View and Projection matrices;
- depth testing, framebuffer resize handling and FPS in the window title.

The buildings, windows and lamps are still simple placeholders. Later weeks can add detailed architecture, trees, a bridge and Astana-inspired landmarks.

## Controls

- Hold the **left mouse button** and drag: orbit around the scene.
- Use the **mouse wheel**: zoom in or out.
- Use the **arrow keys**: orbit with the keyboard (frame-rate independent).
- Press **1**: orbit view; **2**: street-level river view; **3**: aerial view.
- Press **F**: toggle frosted ice patches; **D**: toggle shoreline snow drifts.
- Dragging, scrolling, or using the arrows returns to orbit mode.
- Press **Esc**: close the program.

The window title shows the view shortcuts, frost/drift states, and FPS.

## Project structure

```text
.
├── .vscode/
├── shaders/
│   ├── scene.vert
│   ├── scene.frag
│   ├── snow.vert
│   └── snow.frag
├── src/
│   ├── main.cpp
│   ├── Shader.cpp
│   ├── Shader.h
│   ├── River.cpp
│   └── River.h
├── tests/river_geometry_test.cpp
├── docs/RIVER_HANDOFF.md
├── docs/screenshots/
├── CREDITS.md
├── CMakeLists.txt
└── README.md
```

## Dependencies

- CMake 3.16 or newer
- a C++17 compiler
- GLFW 3.3 or newer
- GLEW
- GLM
- OpenGL 3.3-compatible driver

### macOS with Homebrew

```bash
brew install cmake glfw glew glm
```

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev
```

### Windows with vcpkg

```powershell
vcpkg install glfw3 glew glm
```

When configuring CMake on Windows, select the vcpkg toolchain file in the CMake Tools extension or run:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Run in VS Code

1. Open this folder in VS Code.
2. Install the recommended **C/C++** and **CMake Tools** extensions when VS Code asks.
3. Install the dependencies for your operating system using the commands above.
4. Open the Command Palette with **Cmd+Shift+P** on macOS or **Ctrl+Shift+P** on Windows/Linux.
5. Run **CMake: Select a Kit** and choose your C++ compiler.
6. Run **CMake: Configure**.
7. Press **Cmd+Shift+B** on macOS or **Ctrl+Shift+B** on Windows/Linux to build.
8. Open **Terminal > Run Task > Run Winter Astana**.

You can also use the VS Code terminal:

```bash
cmake -S . -B build
cmake --build build
./build/winter_astana
```

On Windows, the executable may be in `build/Debug/winter_astana.exe` depending on the selected generator.

## Member 4: frozen river and riverbanks

| Part | Contribution | Implementation |
| --- | --- | --- |
| 4 | Frozen river, snowy banks, ice cracks/frost, snow drifts, and view checks | `src/River.h`, `src/River.cpp`, ice/snow sections in `shaders/scene.frag`, and integration in `src/main.cpp` |

`River.cpp` owns the shared shoreline, terrain heights, ice mesh, bank walls, and snow drifts. All seven meshes are generated once and uploaded to static VAO/VBO/EBO buffers. The river has 128 length segments and 32 cross-river subdivisions; its surface relief is less than 0.01 scene units. Area-weighted triangle normals support correct lighting and back-face culling.

The fragment shader receives `uIsIce` and generates world-space procedural texturing: warped cellular fractures at two scales, noise-based frosted patches, extra frost near the banks, Blinn-Phong highlights, and a Fresnel approximation. Frost reduces gloss and softens crack visibility. Derivative-based line filtering reduces distant crack aliasing. Snow terrain and drift meshes use a separate subtle grain treatment through `uIsSnow`. No new image or model library is needed.

See [the Member 4 handoff](docs/RIVER_HANDOFF.md) for the algorithms, integration details, demonstration steps, screenshots, and recorded checks.

## Verification

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/winter_astana --capture-dir build/captures
./build/winter_astana --frames 300 --view orbit
./build/winter_astana --frames 300 --view street
./build/winter_astana --frames 300 --view above
```

For multi-configuration generators on Windows, add `--config Debug` to the build command, `-C Debug` to CTest, and use `build/Debug/winter_astana.exe`.

The geometry test needs no window. It checks finite vertex data, index bounds, triangle winding, unit normals, shoreline seams, inward-facing walls, and snow drifts above the ice. Capture mode renders five deterministic PPM images (overview, street, above, above without frost, above without drifts), using the real OpenGL framebuffer, then exits. PPM files can be opened with an image viewer such as Preview. `--frames` runs the selected view and prints average FPS and OpenGL error status on exit. Capture timings include image readback and are not performance measurements.

On 8 November 2026 the project built and ran on an Apple M1 MacBook Air, macOS 15.0, OpenGL 4.1 Core, at 1280 x 720. Geometry checks passed, all five views were captured without OpenGL errors, and 300-frame samples averaged 109.5 FPS (orbit), 112.2 FPS (street), and 112.8 FPS (above). These are local sample averages, not measurements of the TA machine. Windows/Linux builds have not been run here.

## AI Use Note

OpenAI Codex assisted with the Member 4 river/bank module, procedural ice and snow shader changes, camera inspection modes, geometry tests, documentation, and local build/render checks. The implementation extends the group's existing Week 7 source. No downloaded models, bitmap textures, or third-party shader snippets were added for this contribution. Record any subsequent human review and modifications in the final report's AI Use Note.
