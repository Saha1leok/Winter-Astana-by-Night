cmake_minimum_required(VERSION 3.16)

project(WinterAstanaWeek7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(winter_astana
    src/main.cpp
    src/Shader.cpp
    src/River.cpp
    src/Lighting.cpp
    src/SceneDirector.cpp
    src/Fireworks.cpp
    src/FireworkRenderer.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_executable(reveal_tests tests/reveal_tests.cpp src/SceneDirector.cpp src/Fireworks.cpp)
    target_include_directories(reveal_tests PRIVATE src)
    target_link_libraries(reveal_tests PRIVATE glm::glm)
    add_test(NAME reveal_logic COMMAND reveal_tests)
    option(WINTER_ASTANA_GRAPHICS_TESTS "Run the OpenGL smoke test (requires a display)" OFF)
    if(WINTER_ASTANA_GRAPHICS_TESTS)
        add_test(NAME reveal_graphics COMMAND winter_astana --smoke-test "${CMAKE_CURRENT_BINARY_DIR}/reveal-captures")
        set_tests_properties(reveal_graphics PROPERTIES TIMEOUT 180)
    endif()
endif()

target_include_directories(winter_astana PRIVATE src)
target_link_libraries(winter_astana PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm)

if(MSVC)
    target_compile_options(winter_astana PRIVATE /W4)
else()
    target_compile_options(winter_astana PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(river_geometry_test tests/river_geometry_test.cpp src/River.cpp)
    target_include_directories(river_geometry_test PRIVATE src)
    target_link_libraries(river_geometry_test PRIVATE glm::glm)
    add_test(NAME river_geometry COMMAND river_geometry_test)
endif()

add_custom_command(TARGET winter_astana POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:winter_astana>/shaders"
)

