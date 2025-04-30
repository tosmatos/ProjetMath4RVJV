#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
    // New functions for real-time window movement and clipping
    bool startWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder);
    void handleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder);
    void endWindowDrag();

    // Clipping algorithm wrappers for reuse
    void performCyrusBeckClipping(PolyBuilder& polybuilder);
    void performSutherlandHodgmanClipping(PolyBuilder& polybuilder);
  
	void drawVertexInfoPanel(PolyBuilder& polybuilder, bool* open = nullptr);
	void drawBezierInfoPanel(PolyBuilder& polybuilder, bool* open = nullptr);
	void handleContextMenu(bool* openContextMenu, PolyBuilder& polybuilder);
	void drawHoverTooltip(GLFWwindow* window, PolyBuilder& polybuilder);
	void drawFillSettingsPanel(bool* open = nullptr);
	void handleFillClick(GLFWwindow* window, PolyBuilder& polyBuilder, double xPos, double yPos);
	void handleNonSeedFill(PolyBuilder& polyBuilder);
	void handleSeedFill(PolyBuilder& polyBuilder, float ndcX, float ndcY);
}