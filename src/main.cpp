#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "Polygon.h"
#include "PolyBuilder.h"
#include "Shader.h"

bool openContextMenu;

// Window resize callback
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

/* I've learned the hard way that the glfwSetCallback functions expect a global function.
** You cannot use a function in a class at all, except using some funky middle function...
** So I'm guessing we set up these callbacks here, and then call our class functions
** in the callbacks themselves. */

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) // Quit app
		glfwSetWindowShouldClose(window, true);

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		PolyBuilder::Finish();
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	// TODO : Something for building polygons on left clicks
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		std::cout << "Mouse position : x = " << xPos << ", y = " << yPos << std::endl;
		PolyBuilder::AppendVertex(xPos, yPos);
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		openContextMenu = true;
	}
}

static void setupCallbacks(GLFWwindow* window)
{
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
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

		// TODO : Find a way to put this somewhere else for the code to be cleaner

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		static ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		static ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
		ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
		if (ImGui::Begin("Vertex Info", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (PolyBuilder::finishedPolygons.empty())
			{
				ImGui::Text("No polygons.");
			}
			for (const auto& poly : PolyBuilder::finishedPolygons)
			{
				const auto& verts = poly.getVertices();

				ImGui::ColorButton("Couleur", (poly.type == PolyBuilder::POLYGON) ?	red : green);
				ImGui::SameLine(); //
				ImGui::Text("%s:", (poly.type == PolyBuilder::POLYGON) ? "Polygon" : "Window");

				for (size_t i = 0; i < verts.size(); ++i)
				{

					ImGui::Text("  Vertex %d: (%.2f, %.2f)",
						static_cast<int>(i + 1),
						verts[i].x,
						verts[i].y);
				}
				ImGui::Separator();
			}
		}
		ImGui::End();

		if (openContextMenu)
		{
			ImGui::OpenPopup("ContextMenu");
			openContextMenu = false;  // Reset the flag
		}

		if (ImGui::BeginPopup("ContextMenu"))
		{
			if (ImGui::MenuItem("Create Polygon"))
			{
				PolyBuilder::StartPolygon(PolyBuilder::POLYGON);
			}
			if (ImGui::MenuItem("Create Window"))
			{
				PolyBuilder::StartPolygon(PolyBuilder::WINDOW);
			}
			ImGui::Separator();
			if (PolyBuilder::buildingPoly)
			{
				if (ImGui::MenuItem("Cancel Current Build"))
				{
					PolyBuilder::Cancel();
				}
			}
			ImGui::EndPopup();
		}

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		

		// Draw finished polygons
		for each (const Polygon & poly in PolyBuilder::finishedPolygons)
		{
			shader.Use();
			switch (poly.type) { // Use the polygon's own type
			case PolyBuilder::POLYGON:
				shader.SetColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f);
				break;
			case PolyBuilder::WINDOW:
				shader.SetColor("uColor", 0.0f, 1.0f, 0.0f, 1.0f); 
				break;
			}
			poly.draw();
			shader.SetColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
			poly.drawPoints();
			// Same drawPoints Logic could be used for when points cross between the window and the polygon
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
