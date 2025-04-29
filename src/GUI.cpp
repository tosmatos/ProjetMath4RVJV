#include "GUI.h"

#include <iostream>

#include "Clipper.h"
#include "Filler.h"
#include "PolyTypes.h"

// For filling functionality
static int selectedPolygonIndex = -1;
static bool awaitingFillSeed = false;
static ImVec4 fillColor(0.0f, 0.0f, 1.0f, 1.0f);

// State for window dragging
namespace {
	bool isDraggingWindow = false;
	float lastMouseX = 0.0f;
	float lastMouseY = 0.0f;
	int selectedWindowIndex = -1;
}

void GUI::DrawVertexInfoPanel(PolyBuilder& polybuilder, bool* open) {
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Vertex Info", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (polybuilder.GetFinishedPolygons().empty()) {
			ImGui::Text("No polygons.");
		}

		static ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		static ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
		static ImVec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		static ImVec4 purple(0.8f, 0.0f, 0.8f, 1.0f);

		for (const auto& poly : polybuilder.GetFinishedPolygons()) {
			const auto& verts = poly.getVertices();

			// Show color swatch based on polygon type
			ImVec4 polyColor;
			std::string polyTypeName;

			switch (poly.type) {
			case PolyType::POLYGON:
				polyColor = red;
				polyTypeName = "Polygon";
				break;
			case PolyType::WINDOW:
				polyColor = green;
				polyTypeName = "Window";
				break;
			case PolyType::CLIPPED_CYRUS_BECK:
				polyColor = blue;
				polyTypeName = "Clipped (Cyrus-Beck)";
				break;
			case PolyType::CLIPPED_SUTHERLAND_HODGMAN:
				polyColor = purple;
				polyTypeName = "Clipped (Sutherland-Hodgman)";
				break;
			}

			ImGui::ColorButton("##Color", polyColor);
			ImGui::SameLine();
			ImGui::Text("%s:", polyTypeName.c_str());

			for (size_t i = 0; i < verts.size(); ++i) {
				ImGui::Text("  Vertex %d: (%.2f, %.2f)",
					static_cast<int>(i + 1), verts[i].x, verts[i].y);
			}
			ImGui::Separator();
		}
	}
	ImGui::End();
}

void GUI::DrawBezierInfoPanel(PolyBuilder& polybuilder, bool* open)
{
	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Béziers Info", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (polybuilder.GetFinishedBeziers().empty())
			ImGui::Text("No Bézier curve.");
	}
	ImGui::End();
}

void GUI::DrawFillSettingsPanel(bool* open) {
	ImGui::SetNextWindowPos(ImVec2(220, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Fill Settings", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Algorithm selection
		const char* algorithms[] = { "Simple Scanline", "LCA", "Seed Fill", "Recursive Seed Fill" };
		static int currentAlgorithm = Filler::getSelectedAlgorithm();
		if (ImGui::Combo("Algorithm", &currentAlgorithm, algorithms, IM_ARRAYSIZE(algorithms))) {
			Filler::setSelectedAlgorithm(currentAlgorithm);
		}

		// Fill color picker
		if (ImGui::ColorEdit3("Fill Color", (float*)&fillColor)) {
			Filler::setFillColor(fillColor.x, fillColor.y, fillColor.z, fillColor.w);
		}

		// Status message
		if (awaitingFillSeed) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
				"Click inside polygon to place fill seed");
		}

		// Clear selection button
		if (selectedPolygonIndex != -1 && ImGui::Button("Cancel Fill")) {
			selectedPolygonIndex = -1;
			awaitingFillSeed = false;
		}
	}
	ImGui::End();
}

void GUI::HandleContextMenu(bool* openContextMenu, PolyBuilder& polybuilder)
{
	if (*openContextMenu)
	{
		ImGui::OpenPopup("ContextMenu");
		*openContextMenu = false;
	}

	if (ImGui::BeginPopup("ContextMenu"))
	{
		if (ImGui::MenuItem("Create Polygon"))
		{
			polybuilder.StartPolygon(PolyType::POLYGON);
		}
		if (ImGui::MenuItem("Create Window"))
		{
			polybuilder.StartPolygon(PolyType::WINDOW);
		}
		if (ImGui::MenuItem("Create Bézier"))
		{
			polybuilder.StartBezierCurve();
		}
		ImGui::Separator();
		if (polybuilder.IsBuilding()) {
			if (ImGui::MenuItem("Cancel Current Build")) {
				polybuilder.Cancel();
			}
		}

		// Fill operations menu
		if (ImGui::BeginMenu("Fill Operations")) {
			if (ImGui::MenuItem("Fill All Polygons")) {
				// Clear previous fill results
				polybuilder.ClearFilledPolygons();

				for (auto& poly : polybuilder.GetFinishedPolygons()) {
					if (poly.type == PolyType::POLYGON) {
						std::vector<Vertex> fillPoints;

						switch (Filler::getSelectedAlgorithm()) {
						case Filler::FILL_SCANLINE:
							fillPoints = Filler::fillPolygon(poly);
							break;
						case Filler::FILL_LCA:
							fillPoints = Filler::fillPolygonLCA(poly);
							break;
						case Filler::FILL_SEED:
						case Filler::FILL_SEED_RECURSIVE:
							std::cout << "Seed fill requires selecting a polygon and clicking inside it" << std::endl;
							continue;
						}

						// Get fill color
						float r, g, b, a;
						Filler::getFillColor(r, g, b, a);

						// Store the filled polygon
						polybuilder.AddFilledPolygon(poly, fillPoints, r, g, b, a);
					}
				}
			}

			if (ImGui::MenuItem("Select Polygon to Fill")) {
				// This will allow the next polygon click to be selected for filling
				awaitingFillSeed = true;
			}

			if (ImGui::MenuItem("Clear All Fills")) {
				polybuilder.ClearFilledPolygons();
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Cyrus–Beck Clip All Polygons"))
		{
			PerformCyrusBeckClipping(polybuilder);
		}

		if (ImGui::MenuItem("Sutherland-Hodgman Clip All Polygons"))
		{
			PerformSutherlandHodgmanClipping(polybuilder);
		}

		if (ImGui::MenuItem("Ear Cutting Decomposition")) {
			std::vector<Polygon> newPolygons;
			for (const auto& poly : polybuilder.GetFinishedPolygons()) {
				if (poly.type == PolyType::POLYGON) {
					// Créez une copie du polygone pour le modifier
					Polygon polyCopy = poly;
					std::vector<Polygon> triangles = Clipper::earCutting(polyCopy);
					for (auto& triangle : triangles) {
						triangle.type = PolyType::POLYGON;
						triangle.updateBuffers();
						newPolygons.push_back(triangle);
					}
				}
				else {
					newPolygons.push_back(poly);
				}
			}
			// Remplace l'ancienne collection de polygones
			polybuilder.SetFinishedPolygons(newPolygons);
		}

		ImGui::EndPopup();
	}
}

void GUI::PerformCyrusBeckClipping(PolyBuilder& polybuilder) {
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::WINDOW) {
			windowPoly = poly;
			foundWindow = true;
			break;
		}
	}
	if (!foundWindow) {
		std::cout << "No window polygon to clip against!\n";
		return;
	}

	// Clear any previous clipped results of the same type
	polybuilder.RemoveAllPolygonsOfType(PolyType::CLIPPED_CYRUS_BECK);

	// Add clipped versions of polygons
	for (auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::POLYGON) {
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonCyrusBeck(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty()) {
				clipped.type = PolyType::CLIPPED_CYRUS_BECK;
				clipped.updateBuffers();
				polybuilder.AddFinishedPolygon(clipped);
			}
		}
	}
}

void GUI::PerformSutherlandHodgmanClipping(PolyBuilder& polybuilder) {
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::WINDOW) {
			windowPoly = poly;
			foundWindow = true;
			break;
		}
	}
	if (!foundWindow) {
		std::cout << "No window polygon to clip against!\n";
		return;
	}

	// Clear any previous clipped results of the same type
	polybuilder.RemoveAllPolygonsOfType(PolyType::CLIPPED_SUTHERLAND_HODGMAN);

	// Add clipped versions of polygons
	for (auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::POLYGON) {
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonSutherlandHodgman(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty()) {
				clipped.type = PolyType::CLIPPED_SUTHERLAND_HODGMAN;
				clipped.updateBuffers();
				polybuilder.AddFinishedPolygon(clipped);
			}
		}
	}
}

void GUI::DrawHoverTooltip(GLFWwindow* window, PolyBuilder& polybuilder) {
	ImVec2 mousePos = ImGui::GetMousePos();
	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);

	// Convert mouse pos to NDC
	float ndcX = (2.0f * mousePos.x) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * mousePos.y) / displayH;

	// Check proximity to vertices
	const float hoverRadius = 0.02f;
	for (const auto& poly : polybuilder.GetFinishedPolygons()) {
		for (const auto& vert : poly.getVertices()) {
			float dx = vert.x - ndcX;
			float dy = vert.y - ndcY;
			if (dx * dx + dy * dy < hoverRadius * hoverRadius) {
				ImGui::BeginTooltip();
				ImGui::Text("Position: (%.2f, %.2f)", vert.x, vert.y);
				ImGui::EndTooltip();
				return; // Only show the first match
			}
		}
	}
}

void GUI::HandleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder) {
	// If we're not dragging, do nothing
	if (!isDraggingWindow || selectedWindowIndex < 0)
		return;

	// Get current mouse position
	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);

	// Convert to normalized coordinates
	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);
	float ndcX = (2.0f * xPos) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos) / displayH;

	// Calculate movement delta
	float deltaX = ndcX - lastMouseX;
	float deltaY = ndcY - lastMouseY;

	// Update last position
	lastMouseX = ndcX;
	lastMouseY = ndcY;

	// Apply movement to window polygon
	polybuilder.MovePolygon(selectedWindowIndex, deltaX, deltaY);

	// Update clipped polygons (based on current active clipping algorithm)
	// Check if Sutherland-Hodgman clipped polygons exist
	bool hasShClipped = false;
	for (const auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::CLIPPED_SUTHERLAND_HODGMAN) {
			hasShClipped = true;
			break;
		}
	}

	// Check if Cyrus-Beck clipped polygons exist
	bool hasCbClipped = false;
	for (const auto& poly : polybuilder.GetFinishedPolygons()) {
		if (poly.type == PolyType::CLIPPED_CYRUS_BECK) {
			hasCbClipped = true;
			break;
		}
	}

	// Re-perform the active clipping algorithm(s)
	if (hasShClipped) {
		PerformSutherlandHodgmanClipping(polybuilder);
	}

	if (hasCbClipped) {
		PerformCyrusBeckClipping(polybuilder);
	}
}

bool GUI::StartWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder) {
	// Only allow dragging with middle mouse button or Ctrl+Left mouse
	bool isCtrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
		glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

	bool validDragStart = (mouseButton == GLFW_MOUSE_BUTTON_MIDDLE) ||
		(mouseButton == GLFW_MOUSE_BUTTON_LEFT && isCtrlPressed);

	if (!validDragStart)
		return false;

	// Check if we're hovering over a window polygon
	double xPos, yPos;
	glfwGetCursorPos(window, &xPos, &yPos);

	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);
	float ndcX = (2.0f * xPos) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos) / displayH;

	auto finishedPolys = polybuilder.GetFinishedPolygons();

	// Find a window polygon that contains the mouse position
	for (size_t i = 0; i < finishedPolys.size(); i++) {
		const auto& poly = finishedPolys[i];
		if (poly.type == PolyType::WINDOW) {
			// Simple point-in-polygon test (bounding box for now, can be enhanced)
			const auto& verts = poly.getVertices();
			if (verts.size() > 0) {
				// Create a bounding box
				float minX = verts[0].x;
				float maxX = verts[0].x;
				float minY = verts[0].y;
				float maxY = verts[0].y;

				for (const auto& vert : verts) {
					minX = std::min(minX, vert.x);
					maxX = std::max(maxX, vert.x);
					minY = std::min(minY, vert.y);
					maxY = std::max(maxY, vert.y);
				}

				// Check if point is in bounding box
				if (ndcX >= minX && ndcX <= maxX && ndcY >= minY && ndcY <= maxY) {
					isDraggingWindow = true;
					selectedWindowIndex = static_cast<int>(i);
					lastMouseX = ndcX;
					lastMouseY = ndcY;
					return true;
				}
			}
		}
	}

	return false;
}

void GUI::EndWindowDrag() {
	isDraggingWindow = false;
	selectedWindowIndex = -1;
}

void GUI::HandleFillClick(GLFWwindow* window, PolyBuilder& polyBuilder, double xPos, double yPos) {
	if (!awaitingFillSeed) {
		return;
	}

	// Convert window coordinates to NDC
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float ndcX = (2.0f * xPos / width) - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos / height);

	// First check: are we selecting a polygon?
	if (selectedPolygonIndex == -1) {
		// Find a polygon that contains this point
		const auto& polygons = polyBuilder.GetFinishedPolygons();

		for (int i = 0; i < polygons.size(); i++) {
			const auto& poly = polygons[i];
			if (poly.type == PolyType::POLYGON) {
				// Simple point-in-polygon test
				const auto& vertices = poly.getVertices();

				// Check if point is close to any vertex
				for (const auto& vert : vertices) {
					float dx = vert.x - ndcX;
					float dy = vert.y - ndcY;
					if (dx * dx + dy * dy < 0.1f) {  // Selection radius
						selectedPolygonIndex = i;
						std::cout << "Selected polygon at index " << i << " for filling" << std::endl;

						// If not using seed fill, fill immediately
						if (Filler::getSelectedAlgorithm() != Filler::FILL_SEED) {
							handleNonSeedFill(polyBuilder);
						}
						return;
					}
				}
			}
		}
	}
	else if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED ||
			 Filler::getSelectedAlgorithm() == Filler::FILL_SEED_RECURSIVE) {
		// We have a selected polygon index and we're using seed fill
		handleSeedFill(polyBuilder, ndcX, ndcY);
			 }
}

// Helper method for non-seed fill algorithms
void GUI::handleNonSeedFill(PolyBuilder& polyBuilder) {
	if (!polyBuilder.IsValidPolygonIndex(selectedPolygonIndex)) {
		return;
	}

	Polygon& selectedPolygon = polyBuilder.GetPolygonAt(selectedPolygonIndex);
	std::vector<Vertex> fillPoints;

	if (Filler::getSelectedAlgorithm() == Filler::FILL_SCANLINE) {
		fillPoints = Filler::fillPolygon(selectedPolygon);
	}
	else {
		fillPoints = Filler::fillPolygonLCA(selectedPolygon);
	}

	// Get fill color
	float r, g, b, a;
	Filler::getFillColor(r, g, b, a);

	// Store the filled polygon
	polyBuilder.AddFilledPolygon(selectedPolygon, fillPoints, r, g, b, a);

	// Reset state
	selectedPolygonIndex = -1;
	awaitingFillSeed = false;
}

// Helper method for seed fill algorithms
void GUI::handleSeedFill(PolyBuilder& polyBuilder, float ndcX, float ndcY) {
	if (!polyBuilder.IsValidPolygonIndex(selectedPolygonIndex)) {
		return;
	}

	Polygon& selectedPolygon = polyBuilder.GetPolygonAt(selectedPolygonIndex);
	std::vector<Vertex> fillPoints;

	if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED) {
		fillPoints = Filler::fillFromSeed(selectedPolygon, ndcX, ndcY);
	}
	else {
		fillPoints = Filler::fillFromSeedRecursive(selectedPolygon, ndcX, ndcY);
	}

	// Get fill color
	float r, g, b, a;
	Filler::getFillColor(r, g, b, a);

	// Store the filled polygon
	polyBuilder.AddFilledPolygon(selectedPolygon, fillPoints, r, g, b, a);

	// Reset state
	selectedPolygonIndex = -1;
	awaitingFillSeed = false;
}