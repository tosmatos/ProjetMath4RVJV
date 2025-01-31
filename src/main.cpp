#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include "Polygon.h"

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
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	// TODO : Something for building polygons on left clicks
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		std::cout << "Mouse position : x = " << xPos << ", y = " << yPos << std::endl;
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

	// Example usage for the Polygon class
	// Of course in practice we will actually use functions and events from the mouse to have a polygon's vertices.
	Polygon myPolygon;

	myPolygon.addVertex(-0.5f, 0.5f);    // Add a vertex
	myPolygon.addVertex(0.7f, 0.3f);    // Add another
	myPolygon.addVertex(0.3f, 0.2f);    // And another

	myPolygon.updateBuffers();

	double mouseX, mouseY;

	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Poll events (keyboard, mouse)
		glfwPollEvents();

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (openContextMenu)
		{
			ImGui::OpenPopup("ContextMenu");  // This tells ImGui to open our popup
			openContextMenu = false;  // Reset the flag
		}

		if (ImGui::BeginPopup("ContextMenu"))
		{
			if (ImGui::MenuItem("Option 1"))
			{
				// Handle option 1
			}
			if (ImGui::MenuItem("Option 2"))
			{
				// Handle option 2
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Example poly draw 
		myPolygon.draw();

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