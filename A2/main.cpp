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

#include "RenderContext.h"
#include "Clouds.h"

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ctime>
#include <chrono>
#include <regex>

using namespace std::chrono;

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
Camera camera(glm::vec3(-54.5672455, 1190.89990, 2431.02686), vec3(0.0f, 1.0f, 0.0f), -90, -25);

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

    #pragma region Render setup
    RenderContext* render_context = new RenderContext(&camera, SCR_WIDTH, SCR_HEIGHT);
    render_context->enableAtmosphere();

    glm::vec3 ambientColor = vec3(11.f / 255.f, 28.f / 255.f, 53.f / 255.f);
    render_context->ambientColor = ambientColor;
    render_context->clouds->ambientColor = render_context->ambientColor;

    Light light(directional, glm::vec3(0.f, 0.f, 1.f), glm::vec3(1.f), 10.f, true);
    light.enableCloudShadows(render_context->clouds);

    vector<PBRobj*> objects;
    PBRobj earth("../models/earth/earth.obj", glm::vec3(0.f, 0.f, 0.f), 1000.f);

    earth.Ks = 1.f;
    earth.roughness = 0.75;

    objects.push_back(&earth);

    // Infinite render loop
    float i = 0;
    vec3 directionOld = light.direction;
    bool light_autoRotate = true;
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

        auto time_start = high_resolution_clock::now();

        render_context->SCR_WIDTH = SCR_WIDTH;
        render_context->SCR_HEIGHT = SCR_HEIGHT;

        earth.rotate(vec3(0.f, 1.f, 0.f), 0.01f* glfwGetTime());

        render_context->Draw(objects, light);
        render_context->clouds->time = i;

        auto time_end = high_resolution_clock::now();

        auto duration = duration_cast<milliseconds>(time_end - time_start);

        if (light_autoRotate) {
            light.position = vec3(glm::rotate(glm::mat4(), 0.00005f * (float)glfwGetTime(), glm::vec3(1.f, 0.f, 0.f)) * vec4(light.position, 1.f));
            light.direction = normalize(light.position - vec3(0.f));
        }

        #pragma region GUI
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_Once);

        // render GUI

        ImGui::Begin("Parameters", NULL, window_flags);

        ImGui::Text("Frame Time: %dms", duration);

        if (ImGui::TreeNode("Camera Settings"))
        {
            ImGui::Text("Exposure");
            ImGui::DragFloat("##exposure", &camera.Exposure, 0.01, 0.f, 5.f, "%.2f");

            ImGui::Text("Movement Speed");
            ImGui::DragFloat("##movement", &camera.MovementSpeed, 1.f, 1.f, 500.f, "%.2f");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Lighting Settings"))
        {
            ImGui::Text("Sun Color");
            ImGui::ColorEdit3("##color", (float*)&light.color);

            ImGui::Text("Sun Intensity");
            ImGui::SliderFloat("##intensity", &light.intensity, 1.f, 100.0f, "%.4f");

            ImGui::Text("Silver Intensity");
            ImGui::SliderFloat("##silver_intensity", &render_context->clouds->silver_intensity, 0.f, 10.f, "%.6f");

            ImGui::Text("Silver Spread");
            ImGui::SliderFloat("##silver_spread", &render_context->clouds->silver_spread, 0.f, 0.99f, "%.6f");

            ImGui::Text("Sun Direction");
            ImGui::DragFloat3("##dir", (float*)&light.direction, 0.001, -1.f, 1.f, "%f");

            ImGui::Checkbox("Auto Rotate Sun", &light_autoRotate);

            ImGui::Text("Ambient Color");
            ImGui::ColorEdit3("##acolor", (float*)&render_context->ambientColor);

            render_context->clouds->ambientColor = render_context->ambientColor;

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Atmosphere Settings"))
        {
            ImGui::Text("Rayleigh Scale Height");
            ImGui::SliderFloat("##rsh", &render_context->clouds->scaleHeight_rayleigh, 1.f, 250.f, "%.f");

            ImGui::Text("Mie Scale Height");
            ImGui::SliderFloat("##msh", &render_context->clouds->scaleHeight_mie, 1.f, 10.f, "%.f");

            ImGui::Text("Rayleigh Scattering Intensity");
            ImGui::DragFloat("##ray_intensity", &render_context->clouds->ray_intensity, 0.1, 1.f, 1000.f, "%.f");

            ImGui::Text("Mie Scattering Intensity");
            ImGui::DragFloat("##mie_intensity", &render_context->clouds->mie_intensity, 0.1, 1.f, 1000.f, "%.f");

            ImGui::Text("Absorption Intensity");
            ImGui::DragFloat("##absorption_intensity", &render_context->clouds->absorption_intensity, 0.1, 1.f, 1000.f, "%.f");

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Cloud Settings"))
        {    

            ImGui::Text("Top Density");
            ImGui::SliderFloat("##Top Density", &render_context->clouds->topDensity, 0.01f, 1.0f, "%.4f");

            ImGui::Text("Bottom Density");
            ImGui::SliderFloat("##Bottom Density", &render_context->clouds->bottomDensity, 0.f, 1.0f, "%.4f");

            ImGui::Text("Coverage Intensity");
            ImGui::SliderFloat("##Noise Intensity", &render_context->clouds->noiseIntensity, 0.f, 3.f, "%.4f");

            ImGui::Text("Thickness");
            ImGui::SliderFloat("##Cloud Thickness", &render_context->clouds->thickness, 15.f, 50.f, "%.4f");
            
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
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
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