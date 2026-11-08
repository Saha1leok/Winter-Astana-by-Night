cmake_minimum_required(VERSION 3.16)

project(WinterAstanaWeek7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(OpenGL REQUIRED)
option(WINTER_ASTANA_FETCH_DEPS "Download pinned GLFW, GLEW and GLM sources" OFF)
if(WINTER_ASTANA_FETCH_DEPS)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ONLY_LIBS ON CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579)
    FetchContent_Declare(glew GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git GIT_TAG f456deace7b408655109aaeff71421ef2d3858c6)
    FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863)
    FetchContent_MakeAvailable(glfw glew glm)
    add_library(GLEW::GLEW ALIAS libglew_static)
else()
    find_package(glfw3 3.3 REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glm CONFIG REQUIRED)
endif()

add_executable(winter_astana
    src/main.cpp
    src/Shader.cpp
    src/Lighting.cpp
)

target_include_directories(winter_astana PRIVATE src)
target_link_libraries(winter_astana PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm)

if(MSVC)
    target_compile_options(winter_astana PRIVATE /W4)
else()
    target_compile_options(winter_astana PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET winter_astana POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:winter_astana>/shaders"
)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME lighting_smoke
        COMMAND winter_astana --smoke-test "${CMAKE_CURRENT_BINARY_DIR}/lighting-captures")
endif()
