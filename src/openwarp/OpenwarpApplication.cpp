
#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"
#include "Eigen/Dense"
#include <string>
#include <fstream>
#include <glm/mat4x4.hpp>
#include "util/shader_util.hpp"
#include <glm/gtc/matrix_transform.hpp>
using namespace Openwarp;

#define OBJ_DIR "../resources/"

ImGuiIO OpenwarpApplication::imgui_io;

OpenwarpApplication::OpenwarpApplication(bool debug){
    std::cout << "Initializing Openwarp, debug = " << debug << std::endl;
    std::cout << "Initializing GLFW...." << std::endl;
    if(!glfwInit()) {
        abort();
    }
    std::cout << "Initializing application window...." << std::endl;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    const char* glsl_version = "#version 130";

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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    OpenwarpApplication::imgui_io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    initGL();
}

void OpenwarpApplication::Run(){
    while(!glfwWindowShouldClose(window)) {

        glfwPollEvents();
        OpenwarpApplication::imgui_io = ImGui::GetIO();
        
        if(!OpenwarpApplication::imgui_io.WantCaptureMouse)
            processInput();

        // If it's time to render a frame (based on the desired render frequency)
        // we render, and increment the next render time.
        if(glfwGetTime() >= nextRenderTime){
            
            renderScene();
            nextRenderTime += renderInterval;
            
        }

        // Reproject every frame.
        // If we've already rendered this frame, this
        // will not reproject. This is because we don't resample
        // user input before reprojection; in an XR application,
        // you would resample user pose every time before reprojection
        // for the most up-to-date pose.
        doReprojection();

        drawGUI();
        
        glfwSwapBuffers(window);
    }
}

void OpenwarpApplication::drawGUI(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("My First Tool");

    // Display contents in a scrolling region
    ImGui::TextColored(ImVec4(1,1,0,1), "Important Stuff");
    ImGui::BeginChild("Scrolling");
    for (int n = 0; n < 50; n++)
        ImGui::Text("%04d: Some text", n);
    ImGui::EndChild();
    ImGui::End();


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenwarpApplication::processInput(){
    
    bool isFocused = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if(isFocused) {
        // Query key input for translation input.
        Eigen::Vector3f translation = {
            (float)(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_A)),
            (float)(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_Q)),
            (float)(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) - (float)(glfwGetKey(window, GLFW_KEY_W))
        };

        // Query cursor position for rotation
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        orientation = Eigen::AngleAxisf(xpos / WIDTH, -Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(ypos / HEIGHT, -Eigen::Vector3f::UnitX());

        // Alter position based on user orientation and input.
        position += (orientation * translation) * (glfwGetTime() - lastInputTime);

        lastInputTime = glfwGetTime();
    }
}

void OpenwarpApplication::doReprojection(){
    glBindVertexArray(openwarpVAO);
    glUseProgram(openwarpShaderProgram);
    

    // Upload inverse view matrix (camera matrix) of the rendered frame.
    glUniformMatrix4fv(u_render_inverse_v, 1, GL_FALSE, (GLfloat*)(renderedCameraMatrix.data()));

    // Calculate a fresh camera matrix.
    auto freshCameraMatrix = createCameraMatrix(position, orientation);

    // Compute VP matrix for fresh pose.
    auto freshVP = projection * freshCameraMatrix.inverse();

    // Upload the fresh VP matrix.
    glUniformMatrix4fv(u_warp_vp, 1, GL_FALSE, (GLfloat*)freshVP.eval().data());

    // Render directly to screen. If we were going to send this to a
    // lens undistort shader, we'd create another FBO and render to that.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0,0,WIDTH,HEIGHT);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, mesh_vertices_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, uv));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_indices_vbo);
    glDrawElements(GL_TRIANGLES, mesh_indices.size(), GL_UNSIGNED_INT, NULL);
    
}

void OpenwarpApplication::renderScene(){
    // Render to FBO.
    glBindVertexArray(demoVAO);
    glUseProgram(demoShaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glClearDepth(1);

    // Set up user view matrix.
    // We save the camera matrix, so that when Openwarp runs, it can use both
    // the rendered camera matrix, as well as the updated "fresh" camera matrix.
    renderedCameraMatrix = createCameraMatrix(position, orientation);
    renderedView = renderedCameraMatrix.inverse();

    glUniformMatrix4fv(demoModelViewAttr, 1, GL_FALSE, (GLfloat*)(renderedView.data()));
    glUniformMatrix4fv(demoProjectionAttr, 1, GL_FALSE, (GLfloat*)(projection.data()));

    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTexture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    demoscene.Draw();
    glFlush();
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

    // Create both color and depth render textures
    createRenderTexture(&renderTexture, WIDTH, HEIGHT, false);
    createRenderTexture(&depthTexture, WIDTH, HEIGHT, true);

    // Create FBO that will render to them!
    createFBO(&renderTexture, &renderFBO, &depthTexture, &renderDepthTarget, WIDTH, HEIGHT);

    // Load the .obj-file-based that will be rendered for the demo scene.
    demoscene = ObjScene(std::string(OBJ_DIR), "scene.obj");

    projection = perspective(45.0, (double)(WIDTH)/(double)(HEIGHT), 0.1, 100.0);

    // DEMO rendering initialization
    ////////////////////////////////

    // Initialize and link shader program used to render demo scene
    demoShaderProgram = init_and_link("../resources/shaders/demo.vert", "../resources/shaders/demo.frag");

    // Get demo shader attributes
    demoVertexPosAttr = glGetAttribLocation(demoShaderProgram, "vertexPosition");
    demoModelViewAttr = glGetUniformLocation(demoShaderProgram, "u_modelview");
    demoProjectionAttr = glGetUniformLocation(demoShaderProgram, "u_projection");

    // Create and bind global VAO object
    glGenVertexArrays(1, &demoVAO);
    glBindVertexArray(demoVAO);

    // Openwarp rendering initialization
    ////////////////////////////////

    glGenVertexArrays(1, &openwarpVAO);
    glBindVertexArray(openwarpVAO);

    // Build the reprojection mesh for mesh-based Openwarp
	BuildMesh(MESH_WIDTH, MESH_HEIGHT, mesh_indices, mesh_vertices);

    // Build and link shaders.
	openwarpShaderProgram = init_and_link("../resources/shaders/openwarp_mesh.vert", "../resources/shaders/openwarp_mesh.frag");

    // Get the color + depth samplers
    eye_sampler = glGetUniformLocation(openwarpShaderProgram, "Texture");
    depth_sampler = glGetUniformLocation(openwarpShaderProgram, "_Depth");

    // Get the warp matrix uniforms
    // Inverse V and P matrices of the rendered pose
    u_render_inverse_p = glGetUniformLocation(openwarpShaderProgram, "u_renderInverseP");
    u_render_inverse_v = glGetUniformLocation(openwarpShaderProgram, "u_renderInverseV");
    // VP matrix of the fresh pose
    u_warp_vp = glGetUniformLocation(openwarpShaderProgram, "u_warpVP");

    // Generate, bind, and fill mesh VBOs.
    glGenBuffers(1, &mesh_vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_vertices.size() * sizeof(vertex_t), &mesh_vertices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &mesh_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_indices.size() * sizeof(GLuint), &mesh_indices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Upload the projection matrix (and inverse projection matrix) to the
    // demo and openwarp-mesh programs. Should only need to do this once;
    // we won't be changing this projection matrix at runtime (non-resizeable window)
    glUseProgram(demoShaderProgram);
    glUniformMatrix4fv(demoProjectionAttr, 1, GL_FALSE, (GLfloat*)(projection.data()));
    glUseProgram(openwarpShaderProgram);
    glUniformMatrix4fv(u_render_inverse_p, 1, GL_FALSE, (GLfloat*)(projection.inverse().eval().data()));
    glUseProgram(0);

    return 0;
}

int OpenwarpApplication::cleanupGL(){
    return 0;
}