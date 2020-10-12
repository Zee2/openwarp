
#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"
#include <Eigen/Dense>
#include <string>
#include <fstream>
#include <glm/mat4x4.hpp>
#include "util/shader_util.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace Openwarp;

#define OBJ_DIR "../resources/"

OpenwarpApplication::OpenwarpApplication(bool debug){
    std::cout << "Initializing Openwarp, debug = " << debug << std::endl;
    std::cout << "Initializing GLFW...." << std::endl;
    if(!glfwInit()) {
        abort();
    }
    std::cout << "Initializing application window...." << std::endl;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Openwarp Demo", nullptr, nullptr);
    if(!window) {
        std::cerr << "Failed to create window." << std::endl;
        glfwTerminate();
        abort();
    }
    glfwMakeContextCurrent(window);
    std::cout << "Application window successfully created...." << std::endl;
    is_debug = debug;

    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error(std::string("Could initialize GLEW, error = ") +
                                (const char*)glewGetErrorString(err));
    }
    initGL();
}

void OpenwarpApplication::Run(){
    while(!glfwWindowShouldClose(window)) {

        // Render to FBO.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(demoShaderProgram);

        glBindVertexArray(demoVAO);
        glViewport(0, 0, WIDTH, HEIGHT);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        bool isFocused = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

        if(isFocused) {
            // Query key input for translation input.
            Eigen::Vector3f translation = {
                (float)(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_A)),
                (float)(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_Q)),
                (float)(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_W))
            };

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            orientation = Eigen::AngleAxisf(xpos / WIDTH, -Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(ypos / HEIGHT, -Eigen::Vector3f::UnitX());

            position += (orientation * translation) * (glfwGetTime() - lastFrameTime);
        }

        // Set up user view matrix.
        renderedView = createCameraMatrix(position, orientation).inverse();

        glUniformMatrix4fv(demoModelViewAttr, 1, GL_FALSE, (GLfloat*)(renderedView.data()));
        glUniformMatrix4fv(demoProjectionAttr, 1, GL_FALSE, (GLfloat*)(&projection[0][0]));

        // glBindTexture(GL_TEXTURE_2D, renderTexture);
        // glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTexture, 0);

        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        demoscene.Draw();

        lastFrameTime = glfwGetTime();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}



OpenwarpApplication::~OpenwarpApplication(){
    cleanupGL();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int OpenwarpApplication::initGL(){

    // Vsync enabled.
    glfwSwapInterval(0);

    glEnable              ( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( MessageCallback, 0 );

    createRenderTexture(&renderTexture, WIDTH, HEIGHT);
    createFBO(&renderTexture, &renderFBO, &renderDepthTarget, WIDTH, HEIGHT);

    demoscene = ObjScene(std::string(OBJ_DIR), "scene.obj");

    // Use GLM's perspective function; use Eigen for everything else
    projection = glm::perspective(45.0f, (float)(WIDTH)/(float)(HEIGHT), 0.1f, 100.0f);

    // Initialize and link shader program used to render demo scene
    demoShaderProgram = init_and_link("../resources/shaders/demo.vert", "../resources/shaders/demo.frag");

    // Get demo shader attributes
    demoVertexPosAttr = glGetAttribLocation(demoShaderProgram, "vertexPosition");
    demoModelViewAttr = glGetUniformLocation(demoShaderProgram, "u_modelview");
    demoProjectionAttr = glGetUniformLocation(demoShaderProgram, "u_projection");

    if(glGetError())
        abort();

    // Create and bind global VAO object
    glGenVertexArrays(1, &demoVAO);
    glBindVertexArray(demoVAO);

    return 0;
}

int OpenwarpApplication::cleanupGL(){
    return 0;
}