#include "GUI.h"

#include <iostream>

#include "Clipper.h"
#include "Filler.h"

// For filling functionality
static Polygon* selectedPolygonForFill = nullptr;
static bool awaitingFillSeed = false;
static ImVec4 fillColor(0.0f, 0.0f, 1.0f, 1.0f);

// State for window dragging
namespace {
	bool isDraggingWindow = false;
	float lastMouseX = 0.0f;
	float lastMouseY = 0.0f;
	int selectedWindowIndex = -1;
}

void GUI::DrawVertexInfoPanel(bool* open) {
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Vertex Info", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (PolyBuilder::finishedPolygons.empty()) {
			ImGui::Text("No polygons.");
		}

		static ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		static ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
		static ImVec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		static ImVec4 purple(0.8f, 0.0f, 0.8f, 1.0f);

		for (const auto& poly : PolyBuilder::finishedPolygons) {
			const auto& verts = poly.getVertices();

			// Show color swatch based on polygon type
			ImVec4 polyColor;
			std::string polyTypeName;

			switch (poly.type) {
			case PolyBuilder::POLYGON:
				polyColor = red;
				polyTypeName = "Polygon";
				break;
			case PolyBuilder::WINDOW:
				polyColor = green;
				polyTypeName = "Window";
				break;
			case PolyBuilder::CLIPPED_CYRUS_BECK:
				polyColor = blue;
				polyTypeName = "Clipped (Cyrus-Beck)";
				break;
			case PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN:
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

void GUI::DrawFillSettingsPanel(bool* open) {
	ImGui::SetNextWindowPos(ImVec2(10, 300), ImGuiCond_Always);
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
		if (selectedPolygonForFill != nullptr && ImGui::Button("Cancel Fill")) {
			selectedPolygonForFill = nullptr;
			awaitingFillSeed = false;
		}
	}
	ImGui::End();
}

void GUI::HandleContextMenu(bool* openContextMenu) {
	if (*openContextMenu) {
		ImGui::OpenPopup("ContextMenu");
		*openContextMenu = false;
	}

	if (ImGui::BeginPopup("ContextMenu")) {
		if (ImGui::MenuItem("Create Polygon")) {
			PolyBuilder::StartPolygon(PolyBuilder::POLYGON);
		}
		if (ImGui::MenuItem("Create Window")) {
			PolyBuilder::StartPolygon(PolyBuilder::WINDOW);
		}
		ImGui::Separator();
		if (PolyBuilder::buildingPoly) {
			if (ImGui::MenuItem("Cancel Current Build")) {
				PolyBuilder::Cancel();
			}
		}

		// Fill operations menu
		if (ImGui::BeginMenu("Fill Operations")) {
			if (ImGui::MenuItem("Fill All Polygons")) {
				// Clear previous fill results
				PolyBuilder::ClearFilledPolygons();

				for (auto& poly : PolyBuilder::finishedPolygons) {
					if (poly.type == PolyBuilder::POLYGON) {
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
						PolyBuilder::AddFilledPolygon(poly, fillPoints, r, g, b, a);
					}
				}
			}

			if (ImGui::MenuItem("Select Polygon to Fill")) {
				// This will allow the next polygon click to be selected for filling
				awaitingFillSeed = true;
			}

			if (ImGui::MenuItem("Clear All Fills")) {
				PolyBuilder::ClearFilledPolygons();
			}

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Cyrus–Beck Clip All Polygons"))
		{
			PerformCyrusBeckClipping();
		}

		if (ImGui::MenuItem("Sutherland-Hodgman Clip All Polygons"))
		{
			PerformSutherlandHodgmanClipping();
		}

		if (ImGui::MenuItem("Ear Cutting Decomposition")) {
			std::vector<Polygon> newPolygons;
			for (const auto& poly : PolyBuilder::finishedPolygons) {
				if (poly.type == PolyBuilder::POLYGON) {
					// Créez une copie du polygone pour le modifier
					Polygon polyCopy = poly;
					std::vector<Polygon> triangles = Clipper::earCutting(polyCopy);
					for (auto& triangle : triangles) {
						triangle.type = PolyBuilder::POLYGON;
						triangle.updateBuffers();
						newPolygons.push_back(triangle);
					}
				}
				else {
					newPolygons.push_back(poly);
				}
			}
			// Remplace l'ancienne collection de polygones
			PolyBuilder::finishedPolygons = newPolygons;
		}

		ImGui::EndPopup();
	}
}

void GUI::PerformCyrusBeckClipping() {
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::WINDOW) {
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
	auto it = PolyBuilder::finishedPolygons.begin();
	while (it != PolyBuilder::finishedPolygons.end()) {
		if (it->type == PolyBuilder::CLIPPED_CYRUS_BECK) {
			it = PolyBuilder::finishedPolygons.erase(it);
		}
		else {
			++it;
		}
	}

	// Add clipped versions of polygons
	for (auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::POLYGON) {
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonCyrusBeck(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty()) {
				clipped.type = PolyBuilder::CLIPPED_CYRUS_BECK;
				clipped.updateBuffers();
				PolyBuilder::finishedPolygons.push_back(clipped);
			}
		}
	}
}

void GUI::PerformSutherlandHodgmanClipping() {
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::WINDOW) {
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
	auto it = PolyBuilder::finishedPolygons.begin();
	while (it != PolyBuilder::finishedPolygons.end()) {
		if (it->type == PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN) {
			it = PolyBuilder::finishedPolygons.erase(it);
		}
		else {
			++it;
		}
	}

	// Add clipped versions of polygons
	for (auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::POLYGON) {
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonSutherlandHodgman(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty()) {
				clipped.type = PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN;
				clipped.updateBuffers();
				PolyBuilder::finishedPolygons.push_back(clipped);
			}
		}
	}
}

void GUI::DrawHoverTooltip(GLFWwindow* window) {
	ImVec2 mousePos = ImGui::GetMousePos();
	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);

	// Convert mouse pos to NDC
	float ndcX = (2.0f * mousePos.x) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * mousePos.y) / displayH;

	// Check proximity to vertices
	const float hoverRadius = 0.02f;
	for (const auto& poly : PolyBuilder::finishedPolygons) {
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

void GUI::HandleMouseMove(GLFWwindow* window) {
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
	PolyBuilder::MovePolygon(selectedWindowIndex, deltaX, deltaY);

	// Update clipped polygons (based on current active clipping algorithm)
	// Check if Sutherland-Hodgman clipped polygons exist
	bool hasShClipped = false;
	for (const auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::CLIPPED_SUTHERLAND_HODGMAN) {
			hasShClipped = true;
			break;
		}
	}

	// Check if Cyrus-Beck clipped polygons exist
	bool hasCbClipped = false;
	for (const auto& poly : PolyBuilder::finishedPolygons) {
		if (poly.type == PolyBuilder::CLIPPED_CYRUS_BECK) {
			hasCbClipped = true;
			break;
		}
	}

	// Re-perform the active clipping algorithm(s)
	if (hasShClipped) {
		PerformSutherlandHodgmanClipping();
	}

	if (hasCbClipped) {
		PerformCyrusBeckClipping();
	}
}

bool GUI::StartWindowDrag(GLFWwindow* window, int mouseButton) {
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

	// Find a window polygon that contains the mouse position
	for (size_t i = 0; i < PolyBuilder::finishedPolygons.size(); i++) {
		const auto& poly = PolyBuilder::finishedPolygons[i];
		if (poly.type == PolyBuilder::WINDOW) {
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

void GUI::HandleFillClick(GLFWwindow* window, double xPos, double yPos) {
	if (!awaitingFillSeed) {
		return;
	}

	// Convert window coordinates to NDC
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float ndcX = (2.0f * xPos / width) - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos / height);

	// First check: are we selecting a polygon?
	if (selectedPolygonForFill == nullptr) {
		// Find a polygon that contains this point
		for (auto& poly : PolyBuilder::finishedPolygons) {
			if (poly.type == PolyBuilder::POLYGON) {
				// Simple point-in-polygon test
				// This is a simplified test - in a complete implementation
				// you would use a proper point-in-polygon algorithm
				const auto& vertices = poly.getVertices();

				// For simplicity, just check if point is close to any vertex
				for (const auto& vert : vertices) {
					float dx = vert.x - ndcX;
					float dy = vert.y - ndcY;
					if (dx * dx + dy * dy < 0.1f) {  // Larger radius for selection
						selectedPolygonForFill = &poly;
						std::cout << "Selected polygon for filling" << std::endl;

						// If not using seed fill, fill immediately
						if (Filler::getSelectedAlgorithm() != Filler::FILL_SEED) {
							std::vector<Vertex> fillPoints;

							if (Filler::getSelectedAlgorithm() == Filler::FILL_SCANLINE) {
								fillPoints = Filler::fillPolygon(*selectedPolygonForFill);
							}
							else {
								fillPoints = Filler::fillPolygonLCA(*selectedPolygonForFill);
							}

							// Get fill color
							float r, g, b, a;
							Filler::getFillColor(r, g, b, a);

							// Store the filled polygon
							PolyBuilder::AddFilledPolygon(*selectedPolygonForFill, fillPoints, r, g, b, a);

							// Reset state
							selectedPolygonForFill = nullptr;
							awaitingFillSeed = false;
						}
						return;
					}
				}
			}
		}
	}
	else if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED ||
		Filler::getSelectedAlgorithm() == Filler::FILL_SEED_RECURSIVE) {
		// We have a polygon and we're using seed fill
		std::vector<Vertex> fillPoints;

		if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED) {
			fillPoints = Filler::fillFromSeed(*selectedPolygonForFill, ndcX, ndcY);
		}
		else {
			fillPoints = Filler::fillFromSeedRecursive(*selectedPolygonForFill, ndcX, ndcY);
		}

		// Get fill color
		float r, g, b, a;
		Filler::getFillColor(r, g, b, a);

		// Store the filled polygon
		PolyBuilder::AddFilledPolygon(*selectedPolygonForFill, fillPoints, r, g, b, a);

		// Reset state
		selectedPolygonForFill = nullptr;
		awaitingFillSeed = false;
	}
}