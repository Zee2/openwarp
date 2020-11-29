
#include "openwarp.hpp"
#include "OpenwarpApplication.hpp"
#include "Eigen/Dense"
#include <string>
#include <fstream>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>
#include <fstream>
#include <glm/mat4x4.hpp>
#include "util/shader_util.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "util/lib/stb_image_write.h"

using namespace Openwarp;

#define OBJ_DIR "../resources/"

OpenwarpApplication* OpenwarpApplication::instance;

OpenwarpApplication::OpenwarpApplication(size_t meshSize){

    // For static callbacks.
    OpenwarpApplication::instance = this;

    std::cout << "Initializing Openwarp";
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

    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetScrollCallback(window, scrollCallback);
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

    meshWidth = meshHeight = meshSize;
    // Adjust meshwarp bleed radius according to mesh size.
    bleedRadius = (1.0f/(meshWidth));

    initGL();
}



void OpenwarpApplication::DoFullTestRun(const TestRun& testRun) {

    // Create desired output dir if it doesn't exist
    if(!fs::exists(testRun.outputDir)){
        fs::create_directory(testRun.outputDir);
    }

    // Get timestamp of run.
    std::time_t now_tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm* stamp = gmtime(&now_tt);
    char timestampBuffer[128];
    std::strftime(timestampBuffer, 32, "%d.%m.%Y_%H.%M.%S", stamp); 

    // Create the test run dir with timestamp.
    std::string runDir = testRun.outputDir + "/" + std::string(timestampBuffer);
    std::cout << "testRun.outputDir: " << testRun.outputDir << ", runDir: " << runDir << std::endl;
    fs::create_directory(runDir);

    std::string origin_tag = std::to_string(testRun.startPose.position[0]) + "_"
                            + std::to_string(testRun.startPose.position[1]) + "_"
                            + std::to_string(testRun.startPose.position[2]);

    // Write the test origin to a file.
    std::ofstream info_file;
    info_file.open(runDir + "/run_info.txt");
    info_file << origin_tag;
    info_file.close();

    // Create the ground truth and warp directories
    fs::create_directory(runDir + "/warped");
    fs::create_directory(runDir + "/ground_truth");

    // Render and write reprojected frames
    RunTest(testRun, runDir + "/warped", false, false);

    // Render and write ground truth frames
    RunTest(testRun, runDir + "/ground_truth", true, false);
}

void OpenwarpApplication::RunTest(const TestRun& testRun, std::string runDir, bool isGroundTruth, bool testUsesRay){

    // For GL_RGB8
    GLubyte* fb_data = (GLubyte*)malloc(WIDTH * HEIGHT * 3);

    // Need to flip vertically.
    stbi_flip_vertically_on_write(1);

    // shouldReproject = true makes renderScene draw to renderFBO.
    // shouldReproject = false makes renderScene draw to screen.
    
    if(isGroundTruth) {
        shouldReproject = false;
    } else {
        // If not the ground truth run, we render once
        // so that the reprojection has something to use.
        position = testRun.startPose.position;
        orientation = testRun.startPose.orientation;
        shouldReproject = true;
        renderScene();
    }

    for (auto &test : testRun) {
        position = test.position;
        orientation = test.orientation;

        glfwPollEvents();
        if(glfwWindowShouldClose(window)){
            break;
        }

        // Render
        if (isGroundTruth)
            renderScene();
        else
            doReprojection(testUsesRay);

        // Read pixels out from the screen.
        glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, fb_data);

        std::string filename = runDir + "/"
                                + std::to_string(test.relative_pos[0]) + "_"
                                + std::to_string(test.relative_pos[1]) + "_"
                                + std::to_string(test.relative_pos[2]) + ".png";
        std::cout << "Writing to " << filename << std::endl;
        stbi_write_png(filename.c_str(), WIDTH, HEIGHT, 3, fb_data, WIDTH * sizeof(GLubyte) * 3);

        glfwSwapBuffers(window);
    }
}

void OpenwarpApplication::Run(bool showGUI){
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
        if (shouldReproject)
            doReprojection(useRay);

        if(showGUI)
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
    
    if(showMeshConfig) {
        ImGui::SetNextWindowPos(ImVec2(0, 1024), ImGuiCond_Once, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(300,250), ImGuiCond_Always);
        
        ImGui::Begin("Mesh configuration", &showMeshConfig, ImGuiWindowFlags_NoResize);
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

    if(showRayConfig) {
        ImGui::SetNextWindowPos(ImVec2(300, 1024), ImGuiCond_Once, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(300,250), ImGuiCond_Always);
        
        ImGui::Begin("Raymarch configuration", &showRayConfig, ImGuiWindowFlags_NoResize);
        ImGui::Text("Ray exponent power");
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##1", &rayPower, 0.0f, 1.0f);
        ImGui::Text("Ray step size");
        ImGui::SliderFloat("##2", &rayStepSize, 0.0f, 5.0f);
        ImGui::Text("Ray depth offset");
        ImGui::SliderFloat("##3", &rayDepthOffset, 0.0f, 2.0f);
        ImGui::Text("Occlusion detection threshold");
        ImGui::SliderFloat("##4", &occlusionThreshold, 0.0f, 0.03f);
        ImGui::Text("Occlusion offset");
        ImGui::SliderFloat("##5", &occlusionOffset, 0.0f, 1.0f);
        ImGui::PopItemWidth();
    
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
    ImGui::SetNextWindowSize(ImVec2(350,300), ImGuiCond_Always);
    ImGui::Begin("Stats", NULL, overlay_flags);
    ImGui::Text("Current stats:");
    ImGui::Separator();
    ImGui::Text("Render framerate: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f hz", (float)(1.0f/renderInterval));
    ImGui::Text("Is rendering? ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), shouldRenderScene ? "Yes" : "No");
    ImGui::Text("Presentation framerate: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f hz", (float)presentationFramerate);
    ImGui::Text("Current reprojection algo: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), useRay ? "Raymarch-based" : "Mesh-based");
    ImGui::Text("Is reprojecting? ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), shouldReproject ? "Yes" : "No");
    ImGui::Text("Mesh size: ");
    ImGui::SameLine();
    if(useRay) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "N/A");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d x %d", meshWidth, meshHeight);
    }
    
    if (useVsync == true)
    {
        ImGui::Button(" Vsync ON ");
        if (ImGui::IsItemClicked(0))
        {
            useVsync = !useVsync;
            glfwSwapInterval(useVsync ? 1 : 0);
        }
    } else {
        if (ImGui::Button(" Vsync OFF ")){
            useVsync = true;
            glfwSwapInterval(useVsync ? 1 : 0);
        }
    }
    
    
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    
    ImGui::Text("Controls:");
    ImGui::Separator();
    ImGui::Text("Look: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Mouse");
    ImGui::Text("Move: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WASD, QE");
    ImGui::Text("Change render FPS: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Scroll wheel");
    ImGui::Text("Toggle rendering: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "F");
    ImGui::Text("Switch reprojection type: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "T");
    ImGui::Text("Toggle reprojection: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "R");
    ImGui::Text("Release mouse focus: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Esc");
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
        orientation = Eigen::AngleAxisf(((xpos - xpos_onfocus) + xpos_unfocus) / WIDTH, -Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(((ypos - ypos_onfocus) + ypos_unfocus) / HEIGHT, -Eigen::Vector3f::UnitX());

        // Alter position based on user orientation and input.
        position += (orientation * translation) * (glfwGetTime() - lastInputTime);

        lastInputTime = glfwGetTime();
    }
}

void OpenwarpApplication::doReprojection(bool useRay){

    if(useRay) {
        glBindVertexArray(rayProgram.vao);
        glUseProgram(rayProgram.program);

        // Upload matrices of the rendered frame.
        glUniformMatrix4fv(rayProgram.u_renderPV, 1, GL_FALSE, (GLfloat*)((projection * renderedCameraMatrix.inverse()).eval().data()));

        glUniform3fv(rayProgram.u_warpPos, 1, position.data());
        // Calculate a fresh camera matrix.
        auto freshCameraMatrix = createCameraMatrix(position, orientation);

        // Compute VP matrix for fresh pose.
        // auto freshVP = projection * freshCameraMatrix.inverse();

        glUniformMatrix4fv(rayProgram.u_warpInverseVP, 1, GL_FALSE, (GLfloat*)((freshCameraMatrix * projection.inverse()).eval().data()));

        // Uploade parameter/config uniforms
        glUniform1f(rayProgram.u_power, rayPower);
        glUniform1f(rayProgram.u_stepSize, rayStepSize);
        glUniform1f(rayProgram.u_depthOffset, rayDepthOffset);
        glUniform1f(rayProgram.u_occlusionThreshold, occlusionThreshold);
        glUniform1f(rayProgram.u_occlusionOffset, occlusionOffset);

    } else {
        glBindVertexArray(meshProgram.vao);
        glUseProgram(meshProgram.program);

        // Upload inverse view matrix (camera matrix) of the rendered frame.
        glUniformMatrix4fv(meshProgram.u_renderInverseV, 1, GL_FALSE, (GLfloat*)(renderedCameraMatrix.data()));

        // Calculate a fresh camera matrix.
        auto freshCameraMatrix = createCameraMatrix(position, orientation);

        // Compute VP matrix for fresh pose.
        auto freshVP = projection * freshCameraMatrix.inverse();

        // Upload the fresh VP matrix.
        glUniformMatrix4fv(meshProgram.u_warp_vp, 1, GL_FALSE, (GLfloat*)freshVP.eval().data());

        glUniform1f(meshProgram.u_bleedRadius, bleedRadius);
        glUniform1f(meshProgram.u_bleedTolerance, bleedTolerance);
        glUniform1f(meshProgram.u_debugOpacity, showDebugGrid ? 1.0f : 0.0f);
    }

    // Render directly to screen. If we were going to send this to a
    // lens undistort shader, we'd create another FBO and render to that.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0,0,WIDTH,HEIGHT);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, useRay ? rayProgram.mesh_vertices_vbo : meshProgram.mesh_vertices_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, uv));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, useRay ? rayProgram.mesh_indices_vbo : meshProgram.mesh_indices_vbo);
    glDrawElements(GL_TRIANGLES, useRay ? rayProgram.mesh_indices.size() : meshProgram.mesh_indices.size(), GL_UNSIGNED_INT, NULL);

    
}

void OpenwarpApplication::renderScene(){
    // Render to FBO.
    glBindVertexArray(demoVAO);
    glUseProgram(demoShaderProgram);

    // If reprojection is disabled, we render straight to the screen.
    if (shouldReproject)
        glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
    else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
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

    glfwSwapInterval(useVsync ? 1 : 0);

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

    // Openwarp-mesh rendering initialization
    ////////////////////////////////

    glGenVertexArrays(1, &meshProgram.vao);
    glBindVertexArray(meshProgram.vao);

    // Build the reprojection mesh for mesh-based Openwarp
	BuildMesh(meshWidth, meshHeight, meshProgram.mesh_indices, meshProgram.mesh_vertices);

    // Build and link shaders for openwarp-mesh.
	meshProgram.program = init_and_link("../resources/shaders/openwarp_mesh.vert", "../resources/shaders/openwarp_mesh.frag");

    // Get the color + depth samplers
    meshProgram.eye_sampler = glGetUniformLocation(meshProgram.program, "Texture");
    meshProgram.depth_sampler = glGetUniformLocation(meshProgram.program, "_Depth");

    // Get the warp matrix uniforms
    // Inverse V and P matrices of the rendered pose
    meshProgram.u_renderInverseP = glGetUniformLocation(meshProgram.program, "u_renderInverseP");
    meshProgram.u_renderInverseV = glGetUniformLocation(meshProgram.program, "u_renderInverseV");
    // VP matrix of the fresh pose
    meshProgram.u_warp_vp = glGetUniformLocation(meshProgram.program, "u_warpVP");

    // Mesh edge bleed parameters
    meshProgram.u_bleedRadius = glGetUniformLocation(meshProgram.program, "bleedRadius");
    meshProgram.u_bleedTolerance = glGetUniformLocation(meshProgram.program, "edgeTolerance");

    // Mesh edge bleed parameters
    meshProgram.u_debugOpacity = glGetUniformLocation(meshProgram.program, "u_debugOpacity");

    // Generate, bind, and fill mesh VBOs.
    glGenBuffers(1, &meshProgram.mesh_vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, meshProgram.mesh_vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, meshProgram.mesh_vertices.size() * sizeof(vertex_t), &meshProgram.mesh_vertices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &meshProgram.mesh_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshProgram.mesh_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshProgram.mesh_indices.size() * sizeof(GLuint), &meshProgram.mesh_indices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Openwarp-ray rendering initialization
    //////////////////////////////

    glGenVertexArrays(1, &rayProgram.vao);
    glBindVertexArray(rayProgram.vao);

    // Build the reprojection mesh for ray-based Openwarp
    // 2x2 quad
	BuildMesh(2, 2, rayProgram.mesh_indices, rayProgram.mesh_vertices);

    // Build and link shaders for openwarp-ray.
	rayProgram.program = init_and_link("../resources/shaders/openwarp_ray.vert", "../resources/shaders/openwarp_ray.frag");

    // Get the color + depth samplers
    rayProgram.eye_sampler = glGetUniformLocation(rayProgram.program, "Texture");
    rayProgram.depth_sampler = glGetUniformLocation(rayProgram.program, "_Depth");

    // Get the warp matrix uniforms
    // Inverse V and P matrices of the rendered pose
    rayProgram.u_renderPV = glGetUniformLocation(rayProgram.program, "u_renderPV");
    rayProgram.u_warpInverseVP = glGetUniformLocation(rayProgram.program, "u_warpInverseVP");
    rayProgram.u_warpPos = glGetUniformLocation(rayProgram.program, "u_warpPos");

    rayProgram.u_power = glGetUniformLocation(rayProgram.program, "u_power");
    rayProgram.u_stepSize = glGetUniformLocation(rayProgram.program, "u_stepSize");
    rayProgram.u_depthOffset = glGetUniformLocation(rayProgram.program, "u_depthOffset");
    rayProgram.u_occlusionThreshold = glGetUniformLocation(rayProgram.program, "u_occlusionThreshold");
    rayProgram.u_occlusionOffset = glGetUniformLocation(rayProgram.program, "u_occlusionOffset");


    // Generate, bind, and fill mesh VBOs.
    glGenBuffers(1, &rayProgram.mesh_vertices_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rayProgram.mesh_vertices_vbo);
    glBufferData(GL_ARRAY_BUFFER, rayProgram.mesh_vertices.size() * sizeof(vertex_t), &rayProgram.mesh_vertices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &rayProgram.mesh_indices_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rayProgram.mesh_indices_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rayProgram.mesh_indices.size() * sizeof(GLuint), &rayProgram.mesh_indices.at(0), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Upload the projection matrix (and inverse projection matrix) to the
    // demo and openwarp-mesh programs. Should only need to do this once;
    // we won't be changing this projection matrix at runtime (non-resizeable window)
    glUseProgram(demoShaderProgram);
    glUniformMatrix4fv(demoProjectionAttr, 1, GL_FALSE, (GLfloat*)(projection.data()));
    glUseProgram(meshProgram.program);
    glUniformMatrix4fv(meshProgram.u_renderInverseP, 1, GL_FALSE, (GLfloat*)(projection.inverse().eval().data()));
    glUseProgram(rayProgram.program);
    glUseProgram(0);

    return 0;
}

int OpenwarpApplication::cleanupGL(){
    return 0;
}