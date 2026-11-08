# Topic 7: Winter Astana by Night

Week 7 starter project for Computer Graphics Fundamentals. It uses C++17 and OpenGL 3.3 Core without a game engine.

## Current progress

- two low-poly snow banks with visible snow mounds and uneven height;
- a separate wide, curved frozen-river mesh with a procedural crack/shimmer pattern;
- simple raised embankment walls along the river;
- five optional low-poly buildings for scale and composition;
- animated falling snow particles;
- warm glowing windows and simple lamps along the embankment;
- distance fog and an animated ice shimmer;
- orbit camera controlled by the mouse;
- custom GLSL vertex and fragment shaders;
- VAO, VBO and EBO mesh data;
- Model, View and Projection matrices;
- depth testing, framebuffer resize handling and FPS in the window title.

Buildings remain simple placeholders. Dilnaz adds 300 facade windows with a reproducible mix of occupied and dark rooms, 12 framed lamps, and local warm illumination on the snow. See [DILNAZ.md](DILNAZ.md) for the handoff, build commands and verification.

## Controls

- Press **1**: toggle window lighting.
- Press **2**: toggle street lamps and their local illumination.
- Press **L**: turn all lighting off; press again to turn it on.
- Hold the **left mouse button** and drag: orbit around the scene.
- Use the **mouse wheel**: zoom in or out.
- Press **Esc**: close the program.

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
│   ├── Lighting.cpp
│   ├── Lighting.h
│   └── FrameCapture.h
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

## Week 7 technical notes

The ground, river and buildings are separate meshes. The river center and width change gradually along its length, while the snow banks use a small sine-based height variation. The fragment shader receives `uIsIce`; when it is true, it generates cellular crack lines and a small shimmer from world coordinates, so no texture file is required. The camera position is calculated from yaw, pitch and distance, and `glm::lookAt` creates the View matrix.

## Verification status

The source is intended for OpenGL 3.3 Core and uses only GLFW, GLEW, GLM and standard C++. See the handoff note for the exact checks completed in the preparation environment.
