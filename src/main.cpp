#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Polygon.h"
#include "InputHandler.h"

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

// Input processing
void processInput(GLFWwindow *window) {
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {
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

	// Load OpenGL functions with GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Set viewport and resize callback
	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Example usage for the Polygon class
	// Of course in practice we will actually use functions and events from the mouse to have a polygon's vertices.
	Polygon myPolygon;

	myPolygon.addVertex(-0.5f, 0.5f);    // Add a vertex
	myPolygon.addVertex(0.7f, 0.3f);    // Add another
	myPolygon.addVertex(0.3f, 0.2f);    // And another

	myPolygon.updateBuffers();

	InputHandler inputHandler = InputHandler(window);

	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Input
		processInput(window);

		// Rendering
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Example poly draw 
		myPolygon.draw();

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	glfwTerminate();
	return 0;
}