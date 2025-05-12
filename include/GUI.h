#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
	// For filling functionality
	extern int selectedPolygonIndex;
	extern bool awaitingFillSeed;
	extern ImVec4 fillColor;

	extern bool isDraggingWindow;
	extern float lastMouseX;
	extern float lastMouseY;
	extern int selectedWindowIndex;

	extern bool isDraggingVertex;
	extern bool isShapePolygon;
	extern int selectedShapeIndex;
	extern int selectedVertexIndex;

    // Functions for real-time shape movement and clipping
    bool startWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder);
	bool tryStartVertexDrag(GLFWwindow* window, PolyBuilder& polybuilder, double xPos, double yPos);
    void handleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder);
	void handleWindowDrag(GLFWwindow* window, PolyBuilder& polybuilder, float deltaX, float deltaY);
	void handleVertexDrag(GLFWwindow* window, PolyBuilder& polybuilder, float deltaX, float deltaY);
    void endDrag();
	
	void deleteVertex(GLFWwindow* window, PolyBuilder& polybuilder, double xPos, double yPos);

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