#include "InputHandler.h"

// As you can guess, this class handles the input.
// Constructor sets up the callbacks. The rest should be done in the functions

InputHandler::InputHandler(GLFWwindow* window) : window(window)
{
	// Binds the GLFW callbacks to their respective methods in the class
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPositionCallback);	
}

static void InputHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// TODO : Somethings with the keys ? I don't know yet
}

static void InputHandler::MouseButtonCallback(GFLWwindow* window, int button, int action, int mods)
{
	// TODO : Use this callback to make drawing polygons by clicking possible
	// using our private fields mouse_x and y. 
}

static void InputHandler::CursorPositionCallback(GLFWwindow* window, double xPos, double yPos)
{
	mouse_x = xPos;
	mouse_y = yPos;
}

std::pair<double, double> InputHandler::GetMousePos() const
{
	return { mouse_x, mouse_y };
}