cmake_minimum_required(VERSION 3.10)

project(openwarp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(include/glfw)
add_subdirectory(include/glm)

find_package(Vulkan REQUIRED)
#find_package(GLM REQUIRED)

set(SOURCES
        src/openwarp/main2.cpp
        src/openwarp/OpenwarpApplication.cpp
        src/openwarp/vulkan_util.cpp
        src/openwarp/VulkanInstance.cpp
        src/openwarp/Swapchain.cpp
        src/openwarp/OpenwarpObjects.cpp
        include/openwarp/vulkan_util.hpp
        include/openwarp/openwarp.hpp
        include/openwarp/OpenwarpApplication.hpp
)

set(CMAKE_BUILD_TYPE Debug)

file(GLOB SHADERS src/openwarp/shaders/*.vert src/openwarp/shaders/*.frag)
message(INFO ${SHADERS})

add_executable(main ${SOURCES} ${SHADERS})
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/openwarp)


#target_compile_definitions(main PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(main PRIVATE Vulkan::Vulkan)
target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main glfw)
target_link_libraries(main glm)