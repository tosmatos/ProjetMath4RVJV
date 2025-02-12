#pragma once
#include "Polygon.h"
#include "PolyBuilder.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace GUI {
    void DrawVertexInfoPanel(bool* open = nullptr);
    void HandleContextMenu(bool* openContextMenu);
    void DrawHoverTooltip(GLFWwindow* window);
}