#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Dense>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
#include "testrun.hpp"

class Openwarp::OpenwarpApplication{

    const uint32_t WIDTH = 1024;
    const uint32_t HEIGHT = 1024;

    public:
        OpenwarpApplication(size_t meshSize = 1024);
        ~OpenwarpApplication();

        void Run(bool showGUI);

        void DoFullTestRun(const TestRun& testRun);
        void RunTest(const TestRun& testRun, std::string runDir, bool isGroundTruth, bool testUsesRay);

        static OpenwarpApplication* instance;

    private:

        // Application metadata
        ImGuiIO imgui_io;

        double xpos_onfocus = 0,
                ypos_onfocus = 0,
                xpos_unfocus = 0,
                ypos_unfocus = 0;

        bool showMeshConfig = true;
        bool showRayConfig = true;
        bool useVsync = false;

        // Application resources
        ObjScene demoscene;
        Eigen::Matrix4f projection;
        Eigen::Vector3f position = Eigen::Vector3f(0,0,0);
        Eigen::Quaternionf orientation = Eigen::Quaternionf::Identity();
        Eigen::Matrix4f renderedView;
        Eigen::Matrix4f renderedCameraMatrix;

        double nextRenderTime = 0.0f;
        double renderFPS = 15.0f;
        double renderInterval = (1/renderFPS);

        size_t meshWidth = 1024;
        size_t meshHeight = 1024;

        // Hand-tuned parameters
        float bleedRadius = 0.005f;
        float bleedTolerance = 0.0001f;
           
        // Hand-tuned parameters
        float rayPower = 0.5f;
        float rayStepSize = 0.242f;
        float rayDepthOffset = 0.379f;
        float occlusionThreshold = 0.02f;
        float occlusionOffset = 0.388f;

        bool showDebugGrid = false;

        bool shouldRenderScene = true;
        bool shouldReproject = true;
        bool useRay = false;

        double lastSwapTime;
        double presentationFramerate;

        // GLFW resources
        GLFWwindow* window;
        // Need to init so that we don't
        // get uninitialized errors
        double lastInputTime = glfwGetTime();

        // OpenGL resources

        // Demo render resources
        GLuint renderTexture;
        GLuint depthTexture;
        GLuint renderFBO;
        GLuint renderDepthTarget;
        GLint demoShaderProgram;
        GLuint demoVAO;

        // Demo shader attributes
        GLuint demoVertexPosAttr;
        GLuint demoModelViewAttr;
        GLuint demoProjectionAttr;

        typedef struct owMeshProgram {
            // Reprojection resources
            // Reprojection mesh CPU buffers and GPU VBO handles
            std::vector<vertex_t> mesh_vertices;
            GLuint mesh_vertices_vbo;
            std::vector<GLuint> mesh_indices;
            GLuint mesh_indices_vbo;

            // Color- and depth-samplers for openwarp
            GLint eye_sampler;
            GLint depth_sampler;

            // Mesh edge bleed parameters
            GLint u_bleedRadius;
            GLint u_bleedTolerance;

            // Controls opacity of debug grid overlay
            GLint u_debugOpacity;

            GLuint u_renderInverseP;
            GLuint u_renderInverseV;

            // VP matrix of the fresh pose
            GLuint u_warp_vp;

            GLint program;
            GLuint vao;
        } owMeshProgram;

        owMeshProgram meshProgram;

        typedef struct owRayProgram {
            // Reprojection resources
            // Reprojection mesh CPU buffers and GPU VBO handles
            std::vector<vertex_t> mesh_vertices;
            GLuint mesh_vertices_vbo;
            std::vector<GLuint> mesh_indices;
            GLuint mesh_indices_vbo;

            // Color- and depth-samplers for openwarp
            GLint eye_sampler;
            GLint depth_sampler;

            GLuint u_renderPV;
            GLuint u_warpInverseVP;

            GLuint u_warpPos;

            GLuint u_power;
            GLuint u_stepSize;
            GLuint u_depthOffset;
            GLuint u_occlusionThreshold;
            GLuint u_occlusionOffset;

            GLint program;
            GLuint vao;
        } owRayProgram;

        owRayProgram rayProgram;

        int initGL();
        int cleanupGL();

        void drawGUI();
        void processInput();
        void renderScene();
        void doReprojection(bool useRay);

        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
            OpenwarpApplication::instance->renderFPS += yoffset;
            OpenwarpApplication::instance->renderFPS = abs(OpenwarpApplication::instance->renderFPS);
            OpenwarpApplication::instance->renderInterval = 1.0f/OpenwarpApplication::instance->renderFPS;
        }

        static void mouseClickCallback(GLFWwindow* window, int button, int action, int mods) {
            if(OpenwarpApplication::instance != NULL &&
                OpenwarpApplication::instance->imgui_io.WantCaptureMouse) {
                return;
            }
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                if(OpenwarpApplication::instance != NULL) {
                    glfwGetCursorPos(window, &OpenwarpApplication::instance->xpos_onfocus, &OpenwarpApplication::instance->ypos_onfocus);
                }
            }
        }

        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
            
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                if(OpenwarpApplication::instance != NULL) {

                    // Do some math so that the view doesn't jump
                    // when you re-focus the window.
                    double xpos,ypos;
                    auto instance = OpenwarpApplication::instance;
                    glfwGetCursorPos(window, &xpos, &ypos);
                    instance->xpos_unfocus = (xpos - instance->xpos_onfocus) + instance->xpos_unfocus;
                    instance->ypos_unfocus = (ypos - instance->ypos_onfocus) + instance->ypos_unfocus;
                }
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                
            }

            if (key == GLFW_KEY_F && action == GLFW_PRESS) {
                if(OpenwarpApplication::instance != NULL &&
                    !OpenwarpApplication::instance->imgui_io.WantCaptureMouse) {
                    
                    OpenwarpApplication::instance->shouldRenderScene = !OpenwarpApplication::instance->shouldRenderScene;
                }
            }

            if (key == GLFW_KEY_R && action == GLFW_PRESS) {
                if(OpenwarpApplication::instance != NULL &&
                    !OpenwarpApplication::instance->imgui_io.WantCaptureMouse) {
                    
                    OpenwarpApplication::instance->shouldReproject = !OpenwarpApplication::instance->shouldReproject;
                }
            }

            if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                if(OpenwarpApplication::instance != NULL &&
                    !OpenwarpApplication::instance->imgui_io.WantCaptureMouse) {
                    
                    OpenwarpApplication::instance->useRay = !OpenwarpApplication::instance->useRay;
                }
            }
        }

        Eigen::Matrix4f createCameraMatrix(Eigen::Vector3f position, Eigen::Quaternionf orientation){
            Eigen::Matrix4f cameraMatrix = Eigen::Matrix4f::Identity();
            cameraMatrix.block<3,1>(0,3) = position;
            cameraMatrix.block<3,3>(0,0) = orientation.toRotationMatrix();
            return cameraMatrix;
        }

        int createRenderTexture(GLuint* texture_handle, GLuint width, GLuint height, bool isDepth){

            // Create the texture handle.
            glGenTextures(1, texture_handle);
            glBindTexture(GL_TEXTURE_2D, *texture_handle);

            // We use GL_NEAREST for depth textures for performance reasons.
            // Linear filtering on depth textures is (apparently) costly.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, isDepth ? GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            if(isDepth) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
            }
            else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
            }

            glBindTexture(GL_TEXTURE_2D, 0); // unbind texture

            if(glGetError()){
                return 0;
            } else {
                return 1;
            }
        }

        void createFBO(GLuint* texture_handle, GLuint* fbo, GLuint* depth_texture_handle, GLuint* depth_target, GLuint width, GLuint height){
            // Create a framebuffer to draw some things to the eye texture
            glGenFramebuffers(1, fbo);
            // Bind the FBO as the active framebuffer.
            glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

            glGenRenderbuffers(1, depth_target);
            glBindRenderbuffer(GL_RENDERBUFFER, *depth_target);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Attach color texture to color attachment.
            glBindTexture(GL_TEXTURE_2D, *texture_handle);
            glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *texture_handle, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Attach depth texture to depth attachment.
            glBindTexture(GL_TEXTURE_2D, *depth_texture_handle);
            glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, *depth_texture_handle, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            if(glGetError()){
                abort();
            }

            // Unbind FBO.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Build a rectangular plane.
        void BuildMesh(size_t width, size_t height, std::vector<GLuint>& indices, std::vector<vertex_t>& vertices){
            std::cout << "Generating reprojection mesh, size (" << width << ", " << height << ")" << std::endl;
            // Compute the size of the vectors we'll need to store the
            // data, ahead of time.

            // width and height are not in # of verts, but in # of faces.
            size_t num_indices = 2 * 3 * width * height;
            size_t num_vertices = (width + 1)*(height + 1);

            // Size the vectors accordingly
            indices.resize(num_indices);
            vertices.resize(num_vertices);

            // Build indices.
            for ( size_t y = 0; y < height; y++ ) {
                for ( size_t x = 0; x < width; x++ ) {

                    const int offset = ( y * width + x ) * 6;

                    indices[offset + 0] = (GLuint)( ( y + 0 ) * ( width + 1 ) + ( x + 0 ) );
                    indices[offset + 1] = (GLuint)( ( y + 1 ) * ( width + 1 ) + ( x + 0 ) );
                    indices[offset + 2] = (GLuint)( ( y + 0 ) * ( width + 1 ) + ( x + 1 ) );

                    indices[offset + 3] = (GLuint)( ( y + 0 ) * ( width + 1 ) + ( x + 1 ) );
                    indices[offset + 4] = (GLuint)( ( y + 1 ) * ( width + 1 ) + ( x + 0 ) );
                    indices[offset + 5] = (GLuint)( ( y + 1 ) * ( width + 1 ) + ( x + 1 ) );
                }
            }

            // Build vertices
            for (size_t y = 0; y < height + 1; y++){
                for (size_t x = 0; x < width + 1; x++){

                    size_t index = y * ( width + 1 ) + x;

                    vertices[index].uv[0] = ((float)x / width);
                    vertices[index].uv[1] = ((( height - (float)y) / height));

                    if(x == 0) {
                        vertices[index].uv[0] = -0.5f;
                    }
                    if(x == width) {
                        vertices[index].uv[0] = 1.5f;
                    }

                    if(y == 0) {
                        vertices[index].uv[1] = 1.5f;
                    }
                    if(y == height) {
                        vertices[index].uv[1] = -0.5f;
                    }
                }
            }
        }

        // Perspective matrix construction borrowed from
        // http://spointeau.blogspot.com/2013/12/hello-i-am-looking-at-opengl-3.html
        // I would use GLM, but I'd like to use Eigen for the rest of my computations.
        Eigen::Matrix4f perspective
        (
            float fovy,
            float aspect,
            float zNear,
            float zFar
        )
        {
            assert(aspect > 0);
            assert(zFar > zNear);

            float radf = fovy * (M_PI / 180.0);

            float tanHalfFovy = tan(radf / 2.0);
            Eigen::Matrix4f res = Eigen::Matrix4f::Zero();
            res(0,0) = 1.0 / (aspect * tanHalfFovy);
            res(1,1) = 1.0 / (tanHalfFovy);
            res(2,2) = - (zFar + zNear) / (zFar - zNear);
            res(3,2) = - 1.0;
            res(2,3) = - (2.0 * zFar * zNear) / (zFar - zNear);
            return res;
        }
};