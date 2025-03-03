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

bool openContextMenu;

// Window resize callback
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) // Quit app
		glfwSetWindowShouldClose(window, true);

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		PolyBuilder::Finish();
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS) {
        // Try to start window drag first
        if (GUI::StartWindowDrag(window, button)) {
            return; // Successfully started drag, don't process further
        }

        // If not dragging, handle normal mouse actions
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            double xPos, yPos;
            glfwGetCursorPos(window, &xPos, &yPos);
            std::cout << "Mouse position : x = " << xPos << ", y = " << yPos << std::endl;
            PolyBuilder::AppendVertex(xPos, yPos);
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            openContextMenu = true;
        }
    }

    // Handle drag end
    if (action == GLFW_RELEASE) {
        GUI::EndWindowDrag();
    }
}

static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Forward mouse movement to GUI for drag handling
    GUI::HandleMouseMove(window);
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
	GLFWwindow* window = glfwCreateWindow(800, 600, "Polygon Clipping", nullptr, nullptr);
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
	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);	// Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	const char* vertexShaderPath = "shaders/vertex.glsl";
	const char* fragmentShaderPath = "shaders/fragment.glsl";

	Shader shader = Shader(vertexShaderPath, fragmentShaderPath);

	float maxPointSize[2];
	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, maxPointSize);
	std::cout << "Max point size supported: " << maxPointSize[1] << std::endl;

	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Poll events (keyboard, mouse)
		glfwPollEvents();

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Draw all the ImGui part (in GUI namespace)
		GUI::DrawVertexInfoPanel(); // Top left panel
		GUI::HandleContextMenu(&openContextMenu); // Right click menu
		GUI::DrawHoverTooltip(window); // Tooltip when hovering vertices

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

        // First pass: Draw original and window polygons
        for (const auto& poly : PolyBuilder::finishedPolygons) {
            if (poly.type == PolyBuilder::POLYGON || poly.type == PolyBuilder::WINDOW) {
                shader.Use();
                switch (poly.type) {
                case PolyBuilder::POLYGON:
                    shader.SetColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f); // Red for regular polygons
                    break;
                case PolyBuilder::WINDOW:
                    shader.SetColor("uColor", 0.0f, 1.0f, 0.0f, 1.0f); // Green for window polygons
                    break;
                }
                poly.draw();

                shader.SetColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
                poly.drawPoints();
            }
        }

        // Second pass: Draw clipped polygons with transparency
        for (const auto& poly : PolyBuilder::finishedPolygons) {
            if (poly.type == PolyBuilder::CLIPPED_CYRUS_BECK ||
                poly.type == PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN) {

                shader.Use();
                switch (poly.type) {
                case PolyBuilder::CLIPPED_CYRUS_BECK:
                    shader.SetColor("uColor", 0.0f, 0.0f, 1.0f, 0.7f); // Blue with 70% opacity
                    break;
                case PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN:
                    shader.SetColor("uColor", 0.8f, 0.0f, 0.8f, 0.7f); // Purple with 70% opacity
                    break;
                }
                poly.draw();

                shader.SetColor("uColor", 1.0f, 1.0f, 1.0f, 0.7f);
                poly.drawPoints();
            }
        }

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