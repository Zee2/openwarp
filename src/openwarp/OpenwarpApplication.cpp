
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

OpenwarpApplication* OpenwarpApplication::instance;

OpenwarpApplication::OpenwarpApplication(bool debug){

    // For static callbacks.
    OpenwarpApplication::instance = this;

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
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

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
    imgui_io = ImGui::GetIO();

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
        imgui_io = ImGui::GetIO();
        
        processInput();

        // If it's time to render a frame (based on the desired render frequency)
        // we render, and increment the next render time.
        if(glfwGetTime() >= nextRenderTime){
            
            if(shouldRenderScene) {
                renderScene();
            }
            
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
        presentationFramerate = 1.0/(glfwGetTime() - lastSwapTime);
        lastSwapTime = glfwGetTime();
    }
}

void OpenwarpApplication::drawGUI(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Menu Bar
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::Text("Openwarp");
        if (ImGui::BeginMenu("Configuration"))
        {
            if (ImGui::MenuItem("Mesh-based"))
                showMeshConfig = true;
            if (ImGui::MenuItem("Raymarch-based"))
                showRayConfig = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    
    if(showMeshConfig) {
        ImGui::SetNextWindowSize(ImVec2(300,500), ImGuiCond_Once);
        ImGui::Begin("Reprojection configuration", &showMeshConfig);
        if (ImGui::CollapsingHeader("Edge bleed options", ImGuiTreeNodeFlags_DefaultOpen)){
            ImGui::Text("Edge bleed radius");
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##1", &bleedRadius, 0.0f, 0.05f);
            ImGui::Text("Edge bleed tolerance");
            ImGui::SliderFloat("##2", &bleedTolerance, 0.0f, 0.05f);
            ImGui::PopItemWidth();
        }
        ImGui::Checkbox("Show grid debug overlay", &showDebugGrid);
    
        ImGui::End();
    }
    
    // Stats overlay
    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav |
                                    ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos(ImVec2(imgui_io.DisplaySize.x - 10.0f, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::Begin("Stats", NULL, overlay_flags);
    ImGui::Text("Current stats:");
    ImGui::Separator();
    ImGui::Text("Render framerate: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f hz", (float)(1.0f/renderInterval));
    ImGui::Text("Presentation framerate: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f hz", (float)presentationFramerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenwarpApplication::processInput(){
    
    bool isFocused = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
    if(isFocused) {

        // Toggle scene rendering
        // if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        //     shouldRenderScene = !shouldRenderScene;
        // }

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
    // If we've "disabled" reprojection (for demo purposes)
    // we set the fresh camera matrix to the rendered matrix,
    // which will disable reprojection.
    auto freshCameraMatrix = shouldReproject ? createCameraMatrix(position, orientation) : renderedCameraMatrix;

    // Compute VP matrix for fresh pose.
    auto freshVP = projection * freshCameraMatrix.inverse();

    // Upload the fresh VP matrix.
    glUniformMatrix4fv(u_warp_vp, 1, GL_FALSE, (GLfloat*)freshVP.eval().data());

    glUniform1f(u_bleedRadius, bleedRadius);
    glUniform1f(u_bleedTolerance, bleedTolerance);
    glUniform1f(u_debugOpacity, showDebugGrid ? 1.0f : 0.0f);

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

    // Mesh edge bleed parameters
    u_bleedRadius = glGetUniformLocation(openwarpShaderProgram, "bleedRadius");
    u_bleedTolerance = glGetUniformLocation(openwarpShaderProgram, "edgeTolerance");

    // Mesh edge bleed parameters
    u_debugOpacity = glGetUniformLocation(openwarpShaderProgram, "u_debugOpacity");

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