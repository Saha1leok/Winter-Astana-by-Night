# Topic 7: Winter Astana by Night

Final project for Computer Graphics Fundamentals. It uses C++17 and OpenGL 3.3 Core without a game engine.

## Current progress

- two low-poly snow banks with visible snow mounds and uneven height;
- a separate wide, curved frozen-river mesh with a procedural crack/shimmer pattern;
- simple raised embankment walls along the river;
- detailed low-poly city blocks and Khan Shatyr, Bayterek and Ak Orda landmarks;
- animated falling snow particles;
- warm glowing windows and simple lamps along the embankment;
- animated promenade, bridge, roofline and fir-tree garlands;
- separately switchable Khan Shatyr, Bayterek and Ak Orda accent lighting;
- distance fog and an animated ice shimmer;
- orbit camera controlled by the mouse;
- automatic animated camera rise, sequential block lighting and a fireworks finale;
- custom GLSL vertex and fragment shaders;
- VAO, VBO and EBO mesh data;
- Model, View and Projection matrices;
- an explicit parent-child hierarchy: each lamp bulb inherits the lamp-root transform;
- per-fragment Blinn-Phong ambient, diffuse and specular lighting;
- depth testing, framebuffer resize handling and FPS in the window title.

The reveal continues the `feature/city-layout-modeling` scene and integrates Member 2's lighting from `dilnaz-lighting`. See [INTEGRATION.md](INTEGRATION.md) for the Member 3 particle and Member 5 timing interfaces and current limitations.

## Controls

- Hold the **left mouse button** and drag: orbit around the scene.
- Use the **mouse wheel**: zoom in or out.
- Press **R**: replay the camera reveal from the current view.
- Press **Space**: skip the reveal and immediately regain orbit control.
- Press **1** / **2**: toggle windows / lamps; **L**: toggle all lighting.
- Press **G**: toggle garlands.
- Press **H** / **B** / **K**: toggle Khan Shatyr / Bayterek / Ak Orda lighting.
- Press **-** / **+**: decrease / increase landmark-light brightness.
- Press **Esc**: close the program.

The reveal runs automatically on launch. Mouse orbit and zoom become available after the finale (14.5 seconds), or immediately after **Space**. Lights fade in one block at a time as their skyline probes enter the camera view.

The window title provides an on-screen controls reminder and the live FPS counter.

## Team members and responsibilities

Before submission, the team must replace this note with confirmed names, groups, and truthful responsibilities. Do not assign work to a person who cannot explain it during the defense.

## Project structure

```text
.
├── .vscode/
├── shaders/
│   ├── scene.vert
│   ├── scene.frag
│   ├── snow.vert
│   ├── snow.frag
│   ├── fireworks.vert
│   └── fireworks.frag
├── src/
│   ├── main.cpp
│   ├── Shader.cpp
│   ├── Shader.h
│   ├── Lighting.cpp / Lighting.h
│   ├── SceneDirector.cpp / SceneDirector.h
│   ├── Fireworks.cpp / Fireworks.h
│   ├── FireworkRenderer.cpp / FireworkRenderer.h
│   ├── FrameCapture.h
│   └── SceneNode.h
├── tests/
│   ├── reveal_tests.cpp
│   └── scene_node_tests.cpp
├── INTEGRATION.md
├── RUBRIC_CHECKLIST.md
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

## Technical notes

The ground, river and buildings are separate meshes. The river center and width change gradually along its length, while the snow banks use a small sine-based height variation. The fragment shader receives `uIsIce`; when it is true, it generates cellular crack lines and a small shimmer from world coordinates, so no texture file is required. The camera position is calculated from yaw, pitch and distance, and `glm::lookAt` creates the View matrix.

## Verification status

The source targets OpenGL 3.3 Core. Build and logic tests:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Logic tests run without opening a window and cover input limits, camera handoff, replay/skip, sequential lighting, visibility, timing at 30/60/144 FPS, stalled frames and bounded particle lifetimes. To also run the real OpenGL smoke test on a machine with a display:

```bash
cmake -S . -B build -DWINTER_ASTANA_GRAPHICS_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Or run `./build/winter_astana --smoke-test build/reveal-captures` directly. It renders 15 simulated seconds, captures five BMP frames, checks OpenGL errors, verifies all four blocks and exercises the keyboard and mouse callbacks. Build, logic tests and the OpenGL smoke test were verified on macOS with AppleClang 17. Windows/Linux runtime checks remain for teammates on those platforms.

See [RUBRIC_CHECKLIST.md](RUBRIC_CHECKLIST.md) for the requirement-to-evidence map and remaining submission risks.

## AI Use Note

OpenAI Codex assisted with review of the final-project requirements, build/integration diagnosis, an explicit lamp parent-child transform, per-fragment Blinn-Phong specular-lighting changes, documentation, and local verification. The team must review and understand retained changes, record its own modifications, and include the same specific disclosure in the final report. No claim is made that Codex produced team contributions, TA-machine FPS results, the report, the presentation, or the demo video.
