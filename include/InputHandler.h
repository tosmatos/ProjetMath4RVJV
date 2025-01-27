#pragma once
#include <GLFW/glfw3.h>
#include <utility>

class InputHandler
{
private:
	GLFWwindow* window;
	double mouse_x;
	double mouse_y;

public:
	InputHandler(GLFWwindow* window);// : window(window);

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GFLWwindow* window, int button, int action, int mods);
	static void CursorPositionCallback(GLFWwindow* window, double xPos, double yPos);	

	std::pair<double, double> GetMousePos() const;	
};