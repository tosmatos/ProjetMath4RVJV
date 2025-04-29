#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
    // New functions for real-time window movement and clipping
    bool StartWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder);
    void HandleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder);
    void EndWindowDrag();

    // Clipping algorithm wrappers for reuse
    void PerformCyrusBeckClipping(PolyBuilder& polybuilder);
    void PerformSutherlandHodgmanClipping(PolyBuilder& polybuilder);
  
	void DrawVertexInfoPanel(PolyBuilder& polybuilder, bool* open = nullptr);
	void DrawBezierInfoPanel(PolyBuilder& polybuilder, bool* open = nullptr);
	void HandleContextMenu(bool* openContextMenu, PolyBuilder& polybuilder);
	void DrawHoverTooltip(GLFWwindow* window, PolyBuilder& polybuilder);
	void DrawFillSettingsPanel(bool* open = nullptr);
	void HandleFillClick(GLFWwindow* window, PolyBuilder& polyBuilder, double xPos, double yPos);
	void handleNonSeedFill(PolyBuilder& polyBuilder);
	void handleSeedFill(PolyBuilder& polyBuilder, float ndcX, float ndcY);
}