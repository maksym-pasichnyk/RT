cmake_minimum_required(VERSION 3.21)
project(Game)

add_subdirectory(deps/VFX)
add_subdirectory(deps/glm)
add_subdirectory(deps/entt)

add_library(imgui STATIC
    deps/imgui/imgui.h
    deps/imgui/imgui.cpp
    deps/imgui/imconfig.h
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_internal.h
    deps/imgui/imgui_widgets.cpp
    deps/imgui/misc/cpp/imgui_stdlib.h
    deps/imgui/misc/cpp/imgui_stdlib.cpp
)
set_target_properties(imgui PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(imgui PUBLIC deps/imgui)
target_compile_definitions(imgui PUBLIC -DIMGUI_DEFINE_MATH_OPERATORS)

add_executable(Game
    src/Application.cpp
    src/Application.hpp
    src/Camera.hpp
    src/Core.hpp
    src/Mesh.hpp
    src/DrawList.cpp
    src/DrawList.hpp
    src/ImGuiRenderer.cpp
    src/ImGuiRenderer.hpp
    src/main.cpp
    src/Math.hpp
    src/KeyMapping.hpp
    src/Options.hpp
    src/MouseHandler.hpp
    src/MouseHandler.cpp
    src/PlayerInput.hpp
    src/GameApplication.hpp
    src/GameApplication.cpp
    src/ThreadPool.hpp
        src/stb_image.h src/stb_image.cpp)
set_target_properties(Game PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(Game PRIVATE VFX imgui glm)
target_compile_shaders(Game
    assets/shaders/blit.vert
    assets/shaders/blit.frag
    assets/shaders/imgui.frag
    assets/shaders/imgui.vert
    assets/shaders/default.frag
    assets/shaders/default.vert
    assets/shaders/raytrace.comp
)
target_compile_options(Game PRIVATE
    -DGLM_FORCE_XYZW_ONLY
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
    -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES
)