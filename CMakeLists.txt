cmake_minimum_required(VERSION 3.10)



project(openwarp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("${CMAKE_CXX_COMPILER_ID}")

message("Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
endif()

add_executable(openwarp
        src/openwarp/main.cpp
        src/openwarp/openwarp.hpp
        src/openwarp/OpenwarpApplication.cpp
        src/openwarp/OpenwarpApplication.hpp
        src/openwarp/testrun.cpp
        src/openwarp/testrun.hpp
        src/openwarp/util/lib/stb_image.h
        src/openwarp/util/lib/tiny_obj_loader.h
        src/openwarp/util/obj.hpp
        src/openwarp/util/obj.cpp
        
        # imgui does not support cmake... yet!
        include/imgui/imgui_widgets.cpp
        include/imgui/imgui_draw.cpp
        include/imgui/imgui_impl_glfw.cpp
        include/imgui/imgui_impl_opengl3.cpp
        include/imgui/imgui.cpp
        include/imgui/imgui_demo.cpp
)

# std::filesystem
target_link_libraries(openwarp PUBLIC stdc++fs)

# imgui does not support cmake... yet!
include_directories(include/imgui)

# Header-only libraries
include_directories(include/eigen)

set(CMAKE_BUILD_TYPE Debug)

set_property(TARGET openwarp PROPERTY CXX_STANDARD 17)
target_compile_options(openwarp PRIVATE -Wall)

# glfw
add_subdirectory(include/glfw EXCLUDE_FROM_ALL)
target_link_libraries(openwarp PRIVATE glfw)

# glew
add_definitions(-DGLEW_STATIC)
add_subdirectory(include/glew-cmake EXCLUDE_FROM_ALL)
target_link_libraries(openwarp PRIVATE libglew_static)

# glm
add_subdirectory(include/glm EXCLUDE_FROM_ALL)
target_link_libraries(openwarp PRIVATE glm)

