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

bool openContextMenu;
bool showFillSettings = true;
bool leftMouseDown = false;
double lastClickX = 0, lastClickY = 0;

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
		PolyBuilder::Finish();
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	// Get mouse position
	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		std::cout << "Mouse position : x = " << xPos << ", y = " << yPos << std::endl;

		// Check if in polygon building mode or fill mode
		if (PolyBuilder::buildingPoly) {
			PolyBuilder::AppendVertex(xPos, yPos);
		}
		else {
			// Handle fill click
			lastClickX = xPos;
			lastClickY = yPos;
			leftMouseDown = true;

			// Process fill click
			GUI::HandleFillClick(window, xPos, yPos);
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		leftMouseDown = false;
	}

	if (action == GLFW_PRESS) {
        // Try to start window drag first
        if (GUI::StartWindowDrag(window, button)) {
            return; // Successfully started drag, don't process further
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
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
		GUI::DrawFillSettingsPanel(&showFillSettings); // Fill settings panel

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		// Draw filled polygons first (so they appear behind the outlines)
		for (const auto& filled : PolyBuilder::filledPolygons)
		{
			if (!filled.fillPoints.empty()) {
				fillShader.Use();
				fillShader.SetColor("uColor", filled.colorR, filled.colorG, filled.colorB, filled.colorA);

				// Bind and draw the fill points
				glBindVertexArray(filled.vao);
				glDrawArrays(GL_POINTS, 0, filled.fillPoints.size());
			}
		}
		
        // Draw original and window polygons
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