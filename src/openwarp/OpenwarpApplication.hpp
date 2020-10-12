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
#include "util/obj.hpp"

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

        // Application scene
        ObjScene demoscene;

        // GLFW resources
        GLFWwindow* window;

        // OpenGL resources
        GLuint renderTexture;
        GLuint renderFBO;
        GLuint renderDepthTarget;

        int initGL();
        int cleanupGL();

        int createRenderTexture(GLuint* texture_handle, GLuint width, GLuint height){

            // Create the texture handle.
            glGenTextures(1, texture_handle);
            glBindTexture(GL_TEXTURE_2D, *texture_handle);

            // Set the texture parameters for the texture that the FBO will be
            // mapped into.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

            glBindTexture(GL_TEXTURE_2D, 0); // unbind texture

            if(glGetError()){
                return 0;
            } else {
                return 1;
            }
        }

        void createFBO(GLuint* texture_handle, GLuint* fbo, GLuint* depth_target, GLuint width, GLuint height){
            // Create a framebuffer to draw some things to the eye texture
            glGenFramebuffers(1, fbo);
            // Bind the FBO as the active framebuffer.
            glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

            glGenRenderbuffers(1, depth_target);
            glBindRenderbuffer(GL_RENDERBUFFER, *depth_target);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Bind render texture
            printf("About to bind render texture, texture handle: %d\n", *texture_handle);

            glBindTexture(GL_TEXTURE_2D, *texture_handle);
            glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture_handle, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            // attach a renderbuffer to depth attachment point
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *depth_target);

            if(glGetError()){
                printf("displayCB, error after creating fbo\n");
            }

            // Unbind FBO.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
};