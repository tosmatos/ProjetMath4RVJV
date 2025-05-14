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

	extern float lastMouseX;
	extern float lastMouseY;

	extern bool isDraggingShape;
	extern bool isDraggingVertex;
	extern bool isShapePolygon;
	extern int selectedShapeIndex;
	extern int selectedVertexIndex;

	// For tracking scaling relative to initial shape size
	extern float initialScaleMouseX, initialScaleMouseY;
	extern float initialShapeWidth, initialShapeHeight;

	enum TransformationType
	{
		TRANSLATE,
		SCALE,
		ROTATE,
		SHEAR
	};
	extern TransformationType currentTransformationType;

    // Functions for real-time shape movement and clipping
    bool tryStartShapeDrag(GLFWwindow* window, PolyBuilder& polybuilder, int mods);
	bool tryStartVertexDrag(GLFWwindow* window, PolyBuilder& polybuilder, double xPos, double yPos);
    void handleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder);
	void handleShapeDrag(GLFWwindow* window, PolyBuilder& polybuilder);
	void handleVertexDrag(GLFWwindow* window, PolyBuilder& polybuilderY);
    void endDrag(PolyBuilder& polybuilder);
	
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

	void tryDuplicateVertex(GLFWwindow* window, PolyBuilder& polybuilder);
}