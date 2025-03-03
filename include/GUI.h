#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
    // New functions for real-time window movement and clipping
    bool StartWindowDrag(GLFWwindow* window, int mouseButton);
    void HandleMouseMove(GLFWwindow* window);
    void EndWindowDrag();

    // Clipping algorithm wrappers for reuse
    void PerformCyrusBeckClipping();
    void PerformSutherlandHodgmanClipping();
  
	void DrawVertexInfoPanel(bool* open = nullptr);
	void HandleContextMenu(bool* openContextMenu);
	void DrawHoverTooltip(GLFWwindow* window);
	void DrawFillSettingsPanel(bool* open = nullptr);
	void HandleFillClick(GLFWwindow* window, double xPos, double yPos);
}