#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
	// For filling functionality
	static int selectedPolygonIndex = -1;
	static bool awaitingFillSeed = false;
	static ImVec4 fillColor(0.0f, 0.0f, 1.0f, 1.0f);

	bool isDraggingWindow = false;
	float lastMouseX = 0.0f;
	float lastMouseY = 0.0f;
	int selectedWindowIndex = -1;

	bool isDraggingVertex = false;
	bool isShapePolygon = false;
	int selectedShapeIndex = -1;
	int selectedVertexIndex = -1;

    // Functions for real-time window movement and clipping
    bool startWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder);
    void handleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder);
    void endWindowDrag();

	// Vertex dragging
	bool tryStartVertexDrag(GLFWwindow* window, PolyBuilder polybuilder, double xPos, double yPos);

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
	void drawBuildingHelpTextbox(GLFWwindow* window, bool* open = nullptr);
}