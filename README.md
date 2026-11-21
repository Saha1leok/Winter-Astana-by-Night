# Winter Astana by Night

Final project for Computer Graphics Fundamentals, Topic 7. The application is a real-time low-poly winter city written in C++17 with OpenGL 3.3 Core. It does not use a game engine.

## Implemented features

- Modern OpenGL rendering with VAO, VBO and EBO objects.
- Custom GLSL vertex and fragment shaders.
- Model, View and Projection matrices.
- Per-fragment Blinn-Phong lighting with ambient, diffuse and specular terms.
- Directional light and multiple street-lamp point lights.
- Low-poly buildings, Bayterek, Khan Shatyr and Ak Orda.
- Curved frozen river with procedural ice cracks.
- Uneven snow banks and snow on upward-facing surfaces.
- Animated snowfall with wind drift and particle recycling.
- Separate lamp-root and bulb-child transforms.
- Mouse orbit camera and wheel zoom.
- Window, lamp, garland and landmark lighting controls.
- Automatic camera rise with block-by-block lighting.
- Fireworks finale followed by gradual snow accumulation.
- Live FPS counter in the window title.

## Controls

| Input | Action |
|---|---|
| Left mouse drag | Orbit around the city after the reveal |
| Mouse wheel | Zoom in or out |
| R | Replay the cinematic reveal |
| Space | Skip the reveal and enable orbit control |
| 1 | Toggle building windows |
| 2 | Toggle street lamps |
| L | Toggle all main lights |
| G | Toggle garlands |
| H | Toggle Khan Shatyr lighting |
| B | Toggle Bayterek lighting |
| K | Toggle Ak Orda lighting |
| - / + | Change landmark brightness |
| Esc | Close the application |

The reveal starts automatically. Orbit control becomes available after the finale or immediately after pressing Space.

## Team members and responsibilities

| Team member | Main responsibility |
|---|---|
| Kadirbay Tolganay | City layout, roads, buildings and low-poly landmark models; presentation preparation |
| Kairkulova Dilnaz | Windows, street lighting, emissive effects and independent lighting groups; presentation preparation |
| Zhaskairat Ayaulym | Snowfall particles, integration review, testing and final code fixes |
| Myrzakhan Mereke | Frozen river, riverbanks, procedural ice and report preparation |
| Amantai Zere | Orbit camera, cinematic reveal, block lighting, fireworks timing, report and demo video |

Every member must be able to explain their code and project contribution during the defense.

## Project structure

```text
.
├── .vscode/                  VS Code build and run settings
├── shaders/                  Scene, snow and fireworks shaders
├── src/
│   ├── main.cpp              Window setup and render loop
│   ├── Controls.*            Keyboard and mouse input
│   ├── Mesh.*                VAO, VBO and EBO mesh ownership
│   ├── Terrain.*             Snow banks, river and radial geometry
│   ├── CityScene.*           Buildings, landmarks and street furniture
│   ├── Decorations.*         Animated garlands
│   ├── Lighting.*            Windows and point-light upload
│   ├── SceneDirector.*       Camera rise and reveal timeline
│   ├── Fireworks.*           Firework particle simulation
│   ├── FireworkRenderer.*    Firework rendering
│   ├── RenderUtils.*         Shared draw and matrix helpers
│   ├── SceneNode.h           Parent-child transform hierarchy
│   ├── FrameCapture.h        Evidence-frame capture
│   └── Shader.*              GLSL compilation and linking
├── tests/                    Logic and hierarchy tests
├── CREDITS.md                Libraries, licenses and attribution
├── CMakeLists.txt
└── Final_Report.pdf
```

No generated `build` directory is required in the repository or submission archive.

## Requirements

- CMake 3.16 or newer
- C++17 compiler
- OpenGL 3.3-compatible driver
- GLFW 3.3 or newer
- GLEW
- GLM

### macOS

```bash
brew install cmake glfw glew glm
```

### Ubuntu or Debian

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev
```

### Windows with vcpkg

```powershell
vcpkg install glfw3 glew glm
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Build and run

Run these commands from the project folder:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

macOS and Linux:

```bash
./build/winter_astana
```

Windows:

```powershell
.\build\Release\winter_astana.exe
```

The executable copies the shader directory into its output folder during the build.

## Run in VS Code

1. Open the project folder in VS Code.
2. Install the recommended C/C++ and CMake Tools extensions.
3. Open the Command Palette.
4. Select `CMake: Select a Kit` and choose the installed compiler.
5. Run `CMake: Configure`.
6. Press `Cmd+Shift+B` on macOS or `Ctrl+Shift+B` on Windows and Linux.
7. Select `Terminal > Run Task > Run Winter Astana`.

## Tests

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

To include the real OpenGL smoke test:

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DWINTER_ASTANA_GRAPHICS_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The smoke test runs the reveal, checks for OpenGL errors, verifies block lighting and interactions, and saves seven evidence frames.

## Final deliverables

- Source code and shaders in this repository.
- Final report: `Final_Report.pdf`.
- Presentation and demo video are submitted separately from the repository.
- Demo video link: [Winter Astana by Night](https://youtu.be/sjPAO7QsP9M).
- Dependency and attribution information: `CREDITS.md`.
