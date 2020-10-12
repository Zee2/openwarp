
#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"
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
        glClear(GL_COLOR_BUFFER_BIT);
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
    glfwSwapInterval(1);

    createRenderTexture(&renderTexture, WIDTH, HEIGHT);
    createFBO(&renderTexture, &renderFBO, &renderDepthTarget, WIDTH, HEIGHT);

    demoscene = ObjScene(std::string(OBJ_DIR), "scene.obj");


    return 0;
}

int OpenwarpApplication::cleanupGL(){
    return 0;
}