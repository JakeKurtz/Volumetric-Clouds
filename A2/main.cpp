// Jake Kurtz / ID: 30023688

#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include "PBRobj.h"
#include "Light.h"

#include "Render.h"
#include "Clouds.h"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ctime>
#include <chrono>
#include <regex>

typedef tuple<float*, const char*> tuplePF;
typedef tuple<glm::vec3*, const char*> tuplePV3;
typedef tuple<const char*, const char*> tupleP;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// window settings
int SCR_WIDTH = 1920;
int SCR_HEIGHT = 1080;

// camera settings
Camera camera(glm::vec3(0.f, 6005.f, 0.f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseDown = false;
bool click = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    #pragma region OpenGl INIT

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    const char* glsl_version = "#version 330 core";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CPSC 519 FINAL PROJECT", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL(glfwGetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    #pragma endregion

    #pragma region ImGUI INIT

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoSavedSettings;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    #pragma endregion

    #pragma region Shaders, Models, Textures
    // Build and compile shaders
    /*
    Shader cloudShader("../shaders/cloud_vs.glsl", "../shaders/cloud_fs.glsl");
    Shader cameraDepthShader("../shaders/cameraDepth_vs.glsl", "../shaders/cameraDepth_fs.glsl");
    Shader lightDepthShader("../shaders/lightDepth_vs.glsl", "../shaders/lightDepth_fs.glsl");
    Shader cloudDepthShader("../shaders/cloudDepth_vs.glsl", "../shaders/cloudDepth_fs.glsl");
    Shader atmosphereShader("../shaders/atmosphere_vs.glsl", "../shaders/atmosphere_fs.glsl");
    Shader earthShader("../shaders/earth_vs.glsl", "../shaders/earth_fs.glsl");
    Shader screenShader("../shaders/screen_vs.glsl", "../shaders/screen_fs.glsl");
    Shader sunShader("../shaders/sun_vs.glsl", "../shaders/sun_fs.glsl");
    Shader skyShader("../shaders/sky_vs.glsl", "../shaders/sky_fs.glsl");
    Shader starShader("../shaders/star_vs.glsl", "../shaders/star_fs.glsl");
    */

    // Load objects
    /*
    Model cube("../models/cube/cube.obj");
    Model planet("../models/sphere.obj");
    Model terrain("../models/Mountain.obj");
    Model render_target("../models/screen.obj");
    */
    #pragma endregion
    /*
    // ---- Star buffer ---- //
    unsigned int starFBO;
    glGenFramebuffers(1, &starFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, starFBO);

    glGenTextures(1, &starbufTexture);
    glBindTexture(GL_TEXTURE_2D, starbufTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, starbufTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    #pragma endregion
    */
    vector<PBRobj*> objects;

    Light light(directional, glm::vec3(0,1,0), glm::vec3(0.31f), 31.f, true);

    light.direction = glm::normalize(glm::vec3(1.f, 1.f, 1.f));

    PBRobj sphere1("../models/sphere.obj", glm::vec3(0.f,6005.f,-100.f), 100.f);
    PBRobj sphere2("../models/sphere.obj", glm::vec3(3.f,6010.f,0.f), 1.f);
    PBRobj sphere3("../models/sphere.obj", glm::vec3(-3.f,6015.f,0.f), 1.f);
    PBRobj sun("../models/sphere.obj", glm::vec3(0.f,6000.f,0.f), 100.f);
    PBRobj celestialSphere("../models/celestial/celestial.obj", glm::vec3(0.f), 12000.f);
    PBRobj plane("../models/plane.obj", glm::vec3(0.f,6000.f,0.f), 100.f);
    PBRobj mountain("../models/mountain/Mountain.obj", glm::vec3(0.f,6001.f,0.f), 0.15f);

    Shader starShader("../shaders/star_vs.glsl", "../shaders/star_fs.glsl");
    Shader sunShader("../shaders/sun_vs.glsl", "../shaders/sun_fs.glsl");

    celestialSphere.setShader(starShader);
    sun.setShader(sunShader);

    Render* render = new Render(&camera, 1920, 1080);
    Clouds* clouds = new Clouds(render);

    render->setAtmosphere(clouds);

    //objects.push_back(&sphere1);
    //objects.push_back(&sphere2);
    //objects.push_back(&sphere3);
    objects.push_back(&sun);
    objects.push_back(&celestialSphere);
    objects.push_back(&mountain);

    // Infinite render loop
    float i = 0;
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_3D);

        //glEnable(GL_FRAMEBUFFER_SRGB);

        sun.translate(camera.Position+light.getDir() * 8000.f);

        celestialSphere.translate(camera.Position);

        //clouds->time = i;

        render->Draw(objects, light, camera);
        //glDisable(GL_FRAMEBUFFER_SRGB);

        //clouds->Draw(light, camera);

        #pragma region skyShader
        /*
        skyShader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, noiseTex);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, detailNoiseTex);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, coverageTex);        
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, blueNoise);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, cameraDepthTexture);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, starbufTexture);

        skyShader.setMat4("model", skyModel);
        skyShader.setMat4("projection", projection);
        skyShader.setMat4("view", view);

        skyShader.setVec3("camPos", camera.Position);
        skyShader.setVec3("camDir", camera.Front);
        skyShader.setVec3("camUp", camera.Up);
        skyShader.setVec3("camRight", camera.Right);

        skyShader.setFloat("time", i);

        skyShader.setVec2("iResolution", glm::vec2(SCR_WIDTH * resolution_factor, SCR_HEIGHT * resolution_factor));
        skyShader.setVec3("lightDir", lightDir);
        skyShader.setVec3("planetCenter", glm::vec3(0, 0, 0));
        skyShader.setFloat("lightIntensity", lightIntensity);

        skyShader.setFloat("atmosphereHeight", atmosphereHeight);
        skyShader.setFloat("atmosphereRadius", atmosphereRadius);
        skyShader.setFloat("planetRadius", planetRadius);
        skyShader.setFloat("scaleHeight_rayleigh", scaleHeight_rayleigh);
        skyShader.setFloat("scaleHeight_mie", scaleHeight_mie);

        skyShader.setFloat("ray_intensity", ray_intensity);
        skyShader.setFloat("mie_intensity", mie_intensity);
        skyShader.setFloat("absorption_intensity", absorption_intensity);

        skyShader.setFloat("density", density);
        skyShader.setFloat("bottomDensity", bottomDensity);
        skyShader.setFloat("topDensity", topDensity);
        skyShader.setFloat("g", hg_g);
        skyShader.setFloat("silver_intensity", silver_intensity);
        skyShader.setFloat("silver_spread", silver_spread);

        skyShader.setFloat("coverageIntensity", coverageIntensity);
        skyShader.setFloat("noiseIntensity", noiseIntensity);
        skyShader.setFloat("detailIntensity", detailIntensity);
        skyShader.setFloat("lightIntensity", lightIntensity);

        skyShader.setFloat("coverageScale", coverageScale);
        skyShader.setFloat("noiseScale", noiseScale);
        skyShader.setFloat("detailScale", detailScale);

        skyShader.setVec3("lightColor", lightColor);
        skyShader.setVec3("ambientColor", ambientColor);

        skyShader.setFloat("thickness", thickness);
        skyShader.setFloat("exposure", exposure);

        skyShader.setFloat("cloudTopRoundness", cloudTopRoundness);
        skyShader.setFloat("cloudBottomRoundness", cloudBottomRoundness);

        skyShader.setFloat("cloudRadius", cloudRadius);

        skyShader.setFloat("max_steps", max_steps);

        skyShader.setVec3("cloudCenter", cloudCenter);

        skyShader.setInt("noisetex", 0);
        skyShader.setInt("detailNoiseTex", 1);
        skyShader.setInt("coverageTex", 2);
        skyShader.setInt("blueNoise", 3);
        skyShader.setInt("cameraDepthTexture", 4);
        skyShader.setInt("screenTexture", 5);
        skyShader.setInt("starTex", 6);

        planet.Draw(skyShader);
        */
        #pragma endregion

        #pragma region GUI
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_Once);

        // render GUI

        ImGui::Begin("Parameters", NULL, window_flags);

        //ImGui::Text("Resolution Scale");
        //ImGui::SliderFloat("##resolution_factor", &resolution_factor, 0.1f, 1.f, "%.2f");

        //ImGui::Text("Animation Speed");
        //ImGui::SliderFloat("##animation_speed", &animation_speed, 0.01f, 1.f, "%.5f");

        //ImGui::Text("FOV");
        //ImGui::DragFloat("##fov", &camera.Zoom, 1, 1.f, 90.f, "%.5f");

        ImGui::Text("Roughness: %f", mountain.roughness);
        ImGui::SliderFloat("", &mountain.roughness, 0.001f, 1.0f, "%.3f");

        ImGui::Text("Ka: %f", mountain.Ka);
        ImGui::SliderFloat(" ", &mountain.Ka, 0.0f, 1.0f, "%.2f");

        ImGui::Text("Ks: %f", mountain.Ks);
        ImGui::SliderFloat("  ", &mountain.Ks, 0.0f, 1.0f, "%.2f");

        ImGui::Text("Exposure");
        ImGui::DragFloat("##exposure", &camera.Exposure, 0.01, 0.f, 1.f, "%.2f");
        
        if (ImGui::TreeNode("Atmosphere Settings"))
        {

            ImGui::Text("Atmo Radius");
            ImGui::SliderFloat("##atmosphereRadius", &clouds->atmosphereRadius, 1.0f, 100.f, "%.5f");

            ImGui::Text("Planet Radius");
            ImGui::SliderFloat("##planetRadius", &clouds->planetRadius, 1.f, 500.f, "%.5f");

            ImGui::Text("Cloud Aerial Perspective");
            ImGui::DragFloat("##cloud_ap", &clouds->ap_cloud_intensity, 1.0, 1.0, 1000.0, "%0.2f");
            
            ImGui::Text("World Aerial Perspective");
            ImGui::DragFloat("##world_ap", &clouds->ap_world_intensity, 1.0, 1.0, 1000.0, "%0.2f");

            if (ImGui::TreeNode("Scattering/Absorption Settings"))
            {
                ImGui::Text("Rayleigh Scale Height");
                ImGui::SliderFloat("##rsh", &clouds->scaleHeight_rayleigh, 0.001f, 10.f, "%.5f");

                ImGui::Text("Mie Scale Height");
                ImGui::SliderFloat("##msh", &clouds->scaleHeight_mie, 0.001f, 1.f, "%.5f");

                ImGui::Text("Rayleigh Scattering Intensity");
                ImGui::DragFloat("##ray_intensity", &clouds->ray_intensity, 0.1, 1.f, 1000.f, "%.f");

                ImGui::Text("Mie Scattering Intensity");
                ImGui::DragFloat("##mie_intensity", &clouds->mie_intensity, 0.1, 1.f, 1000.f, "%.f");

                ImGui::Text("Absorption Intensity");
                ImGui::DragFloat("##absorption_intensity", &clouds->absorption_intensity, 0.1, 1.f, 1000.f, "%.f");
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Lighting Settings"))
        {
            ImGui::Text("Sun Color");
            ImGui::ColorEdit3("##color", (float*)&light.color);

            ImGui::Text("Ambient Color");
            ImGui::ColorEdit3("##acolor", (float*)&clouds->ambientColor);

            ImGui::Text("Sun Intensity");
            ImGui::SliderFloat("##intensity", &light.intensity, 1.f, 100.0f, "%.4f");

            ImGui::Text("HG Eccentricity");
            ImGui::SliderFloat("g", &clouds->hg_g, -1.f, 1.f, "%.6f");

            ImGui::Text("Silver Intensity");
            ImGui::SliderFloat("##silver_intensity", &clouds->silver_intensity, 0.f, 10.f, "%.6f");

            ImGui::Text("Silver Spread");
            ImGui::SliderFloat("##silver_spread", &clouds->silver_spread, 0.f, 0.99f, "%.6f");

            ImGui::Text("Sun Direction");
            ImGui::DragFloat3("##dir", (float*)&light.direction, 0.001, -1.f, 1.f, "%f");

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Cloud Settings"))
        {

            ImGui::Text("Attinuation Intensity");
            ImGui::DragFloat("##attinuationScalar", &clouds->attinuationScalar, 0.001f, 0.f, 1.f, "%.5f");            
            
            ImGui::Text("Attinuation Clamp");
            ImGui::DragFloat("##attinuationClamp", &clouds->attinuationClamp, 0.001f, 0.f, 1.f, "%.5f");
            
            if (ImGui::TreeNode("Density"))
            {
                ImGui::Text("Density");
                ImGui::SliderFloat("##Density", &clouds->density, 1.0f, 100.0f, "%.5f");

                ImGui::Text("Top Density");
                ImGui::SliderFloat("##Top Density", &clouds->topDensity, 0.01f, 1.0f, "%.4f");

                ImGui::Text("Bottom Density");
                ImGui::SliderFloat("##Bottom Density", &clouds->bottomDensity, 0.f, 1.0f, "%.4f");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Coverage Noise"))
            {
                ImGui::Text("Coverage Noise Intensity");
                ImGui::DragFloat("##Coverage Intensity", &clouds->coverageIntensity, 0.001, 0.f, 1.f, "%.6f");

                ImGui::Text("Coverage Noise Scale");
                ImGui::SliderFloat("##Coverage Scale", &clouds->coverageScale, 0.001f, 0.01f, "%.6f");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Shape Noise"))
            {
                
                ImGui::Text("Shape Noise Scale");
                ImGui::SliderFloat("##Noise Scale", &clouds->noiseScale, 0.001f, 0.1f, "%.3f");

                ImGui::Text("Shape Noise Intensity");
                ImGui::DragFloat("##Noise Intensity", &clouds->noiseIntensity, 0.01, 0.f, 2.f, "%.6f");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Detial Noise"))
            {
                ImGui::Text("Detail Noise Scale");
                ImGui::SliderFloat("##Detail Scale", &clouds->detailScale, 0.001f, 0.1f, "%.3f");

                ImGui::Text("Detail Noise Intensity");
                ImGui::DragFloat("##Detail Intensity", &clouds->detailIntensity, 0.001, 0.f, 1.f, "%.6f");
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Shape Settings"))
            {
                ImGui::Text("Thickness");
                ImGui::DragFloat("##Cloud Thickness", &clouds->thickness, 0.01, 0.01f, 10.f, "%.6f");

                ImGui::Text("Top Roundness");
                ImGui::SliderFloat("##Top Roundness", &clouds->cloudTopRoundness, 0.f, 1.f, "%.6f");

                ImGui::Text("Bottom Roundness");
                ImGui::SliderFloat("##Bottom Roundness", &clouds->cloudBottomRoundness, 0.f, 1.f, "%.6f");
                ImGui::TreePop();
            }

            ImGui::Text("Cloud Shell Radius");
            ImGui::DragFloat("##cloudRadius", &clouds->cloudRadius, 1.0, 1.f, 10000.f, "%.f");
            
            ImGui::Text("Cloud Shell Center Y offset");
            ImGui::DragFloat("##cloudCenter", &clouds->cloudCenter.y, 1.0, 1.f, 10000.f, "%.f");
            
            ImGui::TreePop();
        }
        
        ImGui::End();

        // Render dear imgui into screen
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        i += 0.001;
    }

    #pragma region Clean up
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    //glDeleteFramebuffers(1, &textureColorbuffer);
    //glDeleteFramebuffers(1, &cameraDepthTexture);
    //glDeleteFramebuffers(1, &starbufTexture);
    //glDeleteFramebuffers(1, &framebuffer);
    #pragma endregion

    return 0;
}

#pragma region Callbacks
void ProcessKeyboard(Camera_Movement dir, float deltaTime)
{
    /*float rotVelocity = rotSpeed * deltaTime;
    if (dir == FORWARD)
    {
        rotation = glm::rotate(rotation, -rotVelocity, glm::vec3(1.0, 0.0, 0.0));
    }
    if (dir == BACKWARD)
    {
        rotation = glm::rotate(rotation, rotVelocity, glm::vec3(1.0, 0.0, 0.0));
    }
    if (dir == LEFT)
    {
        rotation = glm::rotate(rotation, -rotVelocity, glm::vec3(0.0, 1.0, 0.0));
    }
    if (dir == RIGHT)
    {
        rotation = glm::rotate(rotation, rotVelocity, glm::vec3(0.0, 1.0, 0.0));
    }*/
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //Camera controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    //Model controls
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (mouseDown && !(ImGui::GetIO().WantCaptureMouse)) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseDown = true;
            click = true;
        }
        else if (action == GLFW_RELEASE) {
            mouseDown = false;
        }
    }
}
#pragma endregion