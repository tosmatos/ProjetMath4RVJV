#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "Polygon.h"
#include "PolyBuilder.h"
#include "Shader.h"
#include "GUI.h"
#include "Filler.h"
#include "PolyTypes.h"
#include "Bezier.h"

bool openContextMenu;
bool showFillSettings = true;
double lastClickX = 0, lastClickY = 0;
PolyBuilder polybuilder;

// Window resize callback
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // Update Filler screen dimensions
    Filler::init(width, height);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) // Quit app
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        polybuilder.finish();

    if (key == GLFW_KEY_C && action == GLFW_PRESS && polybuilder.isBuilding())
        polybuilder.cancel();

    if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS)
        GUI::tryDuplicateVertex(window, polybuilder);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Get mouse position
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            //std::cout << "Mouse position : x = " << xPos << ", y = " << yPos << std::endl;

            // Check if in polygon building mode or fill mode
            if (polybuilder.isBuilding())
            {
                polybuilder.appendVertex(xPos, yPos);
            }
            else if (GUI::awaitingFillSeed)
            {
                // Handle fill click
                lastClickX = xPos;
                lastClickY = yPos;

                // Process fill click
                GUI::handleFillClick(window, polybuilder, xPos, yPos);
            }
            else
            {
                if (GUI::tryStartVertexDrag(window, polybuilder, xPos, yPos))
                {
                    return; // Successfully found vertex to drag
                }
                else
                {
                    GUI::tryStartShapeDrag(window, polybuilder, mods);
                }
            }
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            openContextMenu = true;

        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            GUI::deleteVertex(window, polybuilder, xPos, yPos);
    }
    else if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            GUI::endDrag(polybuilder);
    }
}
    

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Forward mouse movement to GUI for drag handling
    GUI::handleMouseMove(window, polybuilder);
}

static void setupCallbacks(GLFWwindow* window)
{
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPositionCallback);
}

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure OpenGL version (3.3 core profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Polygon Clipping & Filling", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Setup input callbacks
    setupCallbacks(window);

    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set flag for the "gl_Pointsize = float" line in the vertex shader to work
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Enable blending for semi-transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set viewport and resize callback
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize Filler with current window dimensions
    Filler::init(width, height);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);	// Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    const char* vertexShaderPath = "shaders/vertex.glsl";
    const char* vertexFillShaderPath = "shaders/vertex_fill.glsl";
    const char* fragmentShaderPath = "shaders/fragment.glsl";

    Shader shader = Shader(vertexShaderPath, fragmentShaderPath);
    // Fill shader uses normal point size, vertex shader uses bigger points
    Shader fillShader = Shader(vertexFillShaderPath, fragmentShaderPath);


    float maxPointSize[2];
    glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, maxPointSize);
    std::cout << "Max point size supported: " << maxPointSize[1] << std::endl;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll events (keyboard, mouse)
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw all the ImGui part (in GUI namespace)
        GUI::drawVertexInfoPanel(polybuilder); // Top left panel
        GUI::drawBezierInfoPanel(polybuilder);
        GUI::handleContextMenu(&openContextMenu, polybuilder); // Right click menu
        GUI::drawHoverTooltip(window, polybuilder); // Tooltip when hovering vertices
        GUI::drawFillSettingsPanel(&showFillSettings); // Fill settings panel
        if (polybuilder.isBuilding())
            GUI::drawBuildingHelpTextbox(window);
        GUI::drawTransformationHelpTextbox(window);

        // Rendering
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        // Draw filled polygons first (so they appear behind the outlines)
        for (const auto& filled : polybuilder.getFilledPolygons())
        {
            if (!filled.fillPoints.empty()) {
                fillShader.use();
                fillShader.setColor("uColor", filled.colorR, filled.colorG, filled.colorB, filled.colorA);

                // Bind and draw the fill points
                glBindVertexArray(filled.vao);
                glDrawArrays(GL_POINTS, 0, filled.fillPoints.size());
            }
        }
        
        // Draw original and window polygons
        for (const auto& poly : polybuilder.getFinishedPolygons())
        {
            if (poly.type == PolyType::POLYGON || poly.type == PolyType::WINDOW) {
                shader.use();
                switch (poly.type) {
                case PolyType::POLYGON:
                    shader.setColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f); // Red for regular polygons
                    break;
                case PolyType::WINDOW:
                    shader.setColor("uColor", 0.0f, 1.0f, 0.0f, 1.0f); // Green for window polygons
                    break;
                }
                poly.draw();

                shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
                poly.drawPoints();
            }
        }

        // Second pass: Draw clipped polygons with transparency
        for (const auto& poly : polybuilder.getFinishedPolygons()) {
            if (poly.type == PolyType::CLIPPED_CYRUS_BECK ||
                poly.type == PolyType::CLIPPED_SUTHERLAND_HODGMAN) {

                shader.use();
                switch (poly.type) {
                case PolyType::CLIPPED_CYRUS_BECK:
                    shader.setColor("uColor", 0.0f, 0.0f, 1.0f, 0.7f); // Blue with 70% opacity
                    break;
                case PolyType::CLIPPED_SUTHERLAND_HODGMAN:
                    shader.setColor("uColor", 0.8f, 0.0f, 0.8f, 0.7f); // Purple with 70% opacity
                    break;
                }
                poly.draw();

                shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 0.7f);
                poly.drawPoints();
            }
        }
        
        // Draw previews of shapes being built
        if (polybuilder.isBuilding())
        {
            if (polybuilder.bezierMode)
            {
                polybuilder.tempBezier.drawControlPointsPreview(shader);
                if (polybuilder.tempBezier.getControlPoints().size() > 2)
                    polybuilder.tempBezier.drawGeneratedCurvePreview(shader);
            }				
            else 
                polybuilder.tempPolygon.drawPreview(shader);
        }

        // Draw béziers (free degree)
        for (const auto& bezier : polybuilder.getFinishedBeziers())
        {
            bezier.drawControlPoints(shader);
            bezier.drawGeneratedCurve(shader);
            if (bezier.getShowConvexHull())
                bezier.drawConvexHull(shader);
        }

        // Draw béziers sequences
        for (const auto& bezierSequence : polybuilder.getFinishedBezierSequences())
        {
            bezierSequence.draw(shader);
        }

        polybuilder.drawIntersectionMarkers(shader);

        // ImGui Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Clean up ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Clean up glfw
    glfwTerminate();
    return 0;
}