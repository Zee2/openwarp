#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <cstdint>

#include "openwarp.hpp"

class Openwarp::OpenwarpApplication{

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    public:
        OpenwarpApplication(bool debug);
        ~OpenwarpApplication();

        void Run();

    private:

        // Application metadata
        bool is_debug;

        // GLFW resources
        GLFWwindow* window;

        int initGL();
        int cleanupGL();
};