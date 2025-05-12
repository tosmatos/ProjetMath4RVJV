#include "GUI.h"

#include <iostream>
#include <algorithm>

#include "Clipper.h"
#include "Filler.h"
#include "PolyTypes.h"

namespace GUI
{
	int selectedPolygonIndex = -1;
	bool awaitingFillSeed = false;
	ImVec4 fillColor(0.0f, 0.0f, 1.0f, 1.0f);

	bool isDraggingWindow = false;
	float lastMouseX = 0.0f;
	float lastMouseY = 0.0f;
	int selectedWindowIndex = -1;

	bool isDraggingVertex = false;
	bool isShapePolygon = false;
	int selectedShapeIndex = -1;
	int selectedVertexIndex = -1;
}

void GUI::drawVertexInfoPanel(PolyBuilder& polybuilder, bool* open)
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Vertex Info", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (polybuilder.getFinishedPolygons().empty())
			ImGui::Text("No polygons.");

		static ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
		static ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);
		static ImVec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
		static ImVec4 purple(0.8f, 0.0f, 0.8f, 1.0f);

		for (const auto& poly : polybuilder.getFinishedPolygons())
		{
			const auto& verts = poly.getVertices();

			// Show color swatch based on polygon type
			ImVec4 polyColor;
			std::string polyTypeName;

			switch (poly.type)
			{
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

			for (size_t i = 0; i < verts.size(); ++i)
			{
				ImGui::Text("  Vertex %d: (%.2f, %.2f)",
					static_cast<int>(i + 1), verts[i].x, verts[i].y);
			}
			ImGui::Separator();
		}
	}
	ImGui::End();
}

void GUI::drawBezierInfoPanel(PolyBuilder& polybuilder, bool* open)
{
	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Béziers Info", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (polybuilder.getFinishedBeziers().empty())
			ImGui::Text("No Bézier curve.");

		// Store indices to remove
		std::vector<size_t> indicesToRemove;

		for (size_t index = 0; index < polybuilder.getFinishedBeziers().size(); index++)
		{
			const auto& bezier = polybuilder.getFinishedBeziers()[index];
			float stepSize = bezier.getStepSize();
			int controlPoints = bezier.getControlPoints().size();
			int curvePoints = bezier.getGeneratedCurve().size();
			int algorithm = bezier.getAlgorithm();
			std::string algoString = algorithm == 0 ? "Pascal" : "DeCasteljau";

			ImGui::Text("%d : Step Size = %.3f, Control Points : %d, Curve Points : %d, Algorithm : %s",
				static_cast<int>(index), stepSize, controlPoints, curvePoints, algoString.c_str());

			if (ImGui::Button(("<->##" + std::to_string(index)).c_str()))
				polybuilder.swapBezierAlgorithm(index);

			ImGui::SetItemTooltip("Swap Algorithm");
			ImGui::SameLine();
			if (ImGui::Button(("+##" + std::to_string(index)).c_str()))
				polybuilder.incrementBezierStepSize(index);

			ImGui::SetItemTooltip("Increment Step Size by 0.01");
			ImGui::SameLine();
			if (ImGui::Button(("-##" + std::to_string(index)).c_str()))
				polybuilder.decrementBezierStepSize(index);

			ImGui::SetItemTooltip("Decrement Step Size by 0.01");
			ImGui::SameLine();
			if (ImGui::Button(("X##" + std::to_string(index)).c_str()))
				indicesToRemove.push_back(index);
				
			ImGui::SetItemTooltip("Delete Bézier Curve");

			ImGui::Separator();
		}

		// Remove curves marked for deletion (from highest index to lowest)
		// Done afterwards to guarantee that vector won't be changed during loop causing crash
		std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<size_t>());
		for (size_t index : indicesToRemove)
		{
			std::cout << "Removing bézier curve " << index << "..." << std::endl;
			polybuilder.removeFinishedBezier(index);
		}
	}
	ImGui::End();
}

void GUI::drawFillSettingsPanel(bool* open)
{
	ImGui::SetNextWindowPos(ImVec2(220, 10), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Fill Settings", open,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Algorithm selection
		const char* algorithms[] = { "Simple Scanline", "LCA", "Seed Fill", "Recursive Seed Fill" };
		static int currentAlgorithm = Filler::getSelectedAlgorithm();

		if (ImGui::Combo("Algorithm", &currentAlgorithm, algorithms, IM_ARRAYSIZE(algorithms)))
			Filler::setSelectedAlgorithm(currentAlgorithm);

		// Fill color picker
		if (ImGui::ColorEdit3("Fill Color", (float*)&fillColor))
			Filler::setFillColor(fillColor.x, fillColor.y, fillColor.z, fillColor.w);

		// Status message
		if (awaitingFillSeed)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
				"Click inside polygon to place fill seed");
		}

		// Clear selection button
		if (selectedPolygonIndex != -1 && ImGui::Button("Cancel Fill"))
		{
			selectedPolygonIndex = -1;
			awaitingFillSeed = false;
		}
	}
	ImGui::End();
}

void GUI::handleContextMenu(bool* openContextMenu, PolyBuilder& polybuilder)
{
	if (*openContextMenu)
	{
		ImGui::OpenPopup("ContextMenu");
		*openContextMenu = false;
	}

	if (ImGui::BeginPopup("ContextMenu"))
	{
		if (ImGui::MenuItem("Create Polygon"))
			polybuilder.startPolygon(PolyType::POLYGON);

		if (ImGui::MenuItem("Create Window"))
			polybuilder.startPolygon(PolyType::WINDOW);

		if (ImGui::MenuItem("Create Bézier"))
			polybuilder.startBezierCurve();

		ImGui::Separator();
		if (polybuilder.isBuilding())
		{
			if (ImGui::MenuItem("Cancel Current Build"))
				polybuilder.cancel();
		}

		// Fill operations menu
		if (ImGui::BeginMenu("Fill Operations"))
		{
			if (ImGui::MenuItem("Fill All Polygons"))
			{
				// Clear previous fill results
				polybuilder.clearFilledPolygons();

				for (auto& poly : polybuilder.getFinishedPolygons())
				{
					if (poly.type == PolyType::POLYGON)
					{
						std::vector<Vertex> fillPoints;

						switch (Filler::getSelectedAlgorithm())
						{
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
						polybuilder.addFilledPolygon(poly, fillPoints, r, g, b, a);
					}
				}
			}

			if (ImGui::MenuItem("Select Polygon to Fill"))
			{
				// This will allow the next polygon click to be selected for filling
				awaitingFillSeed = true;
			}

			if (ImGui::MenuItem("Clear All Fills"))
				polybuilder.clearFilledPolygons();

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Cyrus–Beck Clip All Polygons"))
			performCyrusBeckClipping(polybuilder);

		if (ImGui::MenuItem("Sutherland-Hodgman Clip All Polygons"))
			performSutherlandHodgmanClipping(polybuilder);

		if (ImGui::MenuItem("Ear Cutting Decomposition"))
		{
			std::vector<Polygon> newPolygons;
			for (const auto& poly : polybuilder.getFinishedPolygons())
			{
				if (poly.type == PolyType::POLYGON)
				{
					// Créez une copie du polygone pour le modifier
					Polygon polyCopy = poly;
					std::vector<Polygon> triangles = Clipper::earCutting(polyCopy);
					for (auto& triangle : triangles)
					{
						triangle.type = PolyType::POLYGON;
						triangle.updateBuffers();
						newPolygons.push_back(triangle);
					}
				}
				else
				{
					newPolygons.push_back(poly);
				}
			}
			// Remplace l'ancienne collection de polygones
			polybuilder.setFinishedPolygons(newPolygons);
		}

		ImGui::EndPopup();
	}
}

void GUI::performCyrusBeckClipping(PolyBuilder& polybuilder)
{
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::WINDOW)
		{
			windowPoly = poly;
			foundWindow = true;
			break;
		}
	}
	if (!foundWindow)
	{
		std::cout << "No window polygon to clip against!\n";
		return;
	}

	// Clear any previous clipped results of the same type
	polybuilder.removeAllPolygonsOfType(PolyType::CLIPPED_CYRUS_BECK);

	// Add clipped versions of polygons
	for (auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::POLYGON)
		{
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonCyrusBeck(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty())
			{
				clipped.type = PolyType::CLIPPED_CYRUS_BECK;
				clipped.updateBuffers();
				polybuilder.addFinishedPolygon(clipped);
			}
		}
	}
}

void GUI::performSutherlandHodgmanClipping(PolyBuilder& polybuilder)
{
	Polygon windowPoly;
	bool foundWindow = false;
	for (auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::WINDOW)
		{
			windowPoly = poly;
			foundWindow = true;
			break;
		}
	}
	if (!foundWindow)
	{
		std::cout << "No window polygon to clip against!\n";
		return;
	}

	// Clear any previous clipped results of the same type
	polybuilder.removeAllPolygonsOfType(PolyType::CLIPPED_SUTHERLAND_HODGMAN);

	// Add clipped versions of polygons
	for (auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::POLYGON)
		{
			// Clip the polygon and add as a new polygon
			Polygon clipped = Clipper::clipPolygonSutherlandHodgman(poly, windowPoly);

			// Only add non-empty clipped polygons
			if (!clipped.getVertices().empty())
			{
				clipped.type = PolyType::CLIPPED_SUTHERLAND_HODGMAN;
				clipped.updateBuffers();
				polybuilder.addFinishedPolygon(clipped);
			}
		}
	}
}

void GUI::drawHoverTooltip(GLFWwindow* window, PolyBuilder& polybuilder)
{
	ImVec2 mousePos = ImGui::GetMousePos();
	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);

	// Convert mouse pos to NDC
	float ndcX = (2.0f * mousePos.x) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * mousePos.y) / displayH;

	// Check proximity to vertices
	const float hoverRadius = 0.02f;
	for (const auto& poly : polybuilder.getFinishedPolygons())
	{
		bool foundMatch = false;
		for (const auto& vert : poly.getVertices())
		{
			float dx = vert.x - ndcX;
			float dy = vert.y - ndcY;
			if (dx * dx + dy * dy < hoverRadius * hoverRadius)
			{
				ImGui::BeginTooltip();
				ImGui::Text("Position: (%.2f, %.2f)", vert.x, vert.y);
				ImGui::EndTooltip();
				foundMatch = true;
				break; // Only show the first match
			}
		}

		if (foundMatch) break;
	}

	for (const auto& bezier : polybuilder.getFinishedBeziers())
	{
		bool foundMatch = false;
		for (const auto& vert : bezier.getControlPoints())
		{
			float dx = vert.x - ndcX;
			float dy = vert.y - ndcY;
			if (dx * dx + dy * dy < hoverRadius * hoverRadius)
			{
				ImGui::BeginTooltip();
				ImGui::Text("Position: (%.2f, %.2f)", vert.x, vert.y);
				ImGui::EndTooltip();
				foundMatch = true;
				break; // Only show the first match
			}
		}

		if (foundMatch) break;
	}
}

void GUI::handleMouseMove(GLFWwindow* window, PolyBuilder& polybuilder)
{
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

	if (isDraggingWindow && selectedWindowIndex != -1)
		handleWindowDrag(window, polybuilder, deltaX, deltaY);
	else if (isDraggingVertex && selectedShapeIndex != -1)
		handleVertexDrag(window, polybuilder, deltaX, deltaY);
}

void GUI::handleWindowDrag(GLFWwindow* window, PolyBuilder& polybuilder, float deltaX, float deltaY)
{
	// If we're not dragging, do nothing
	if (!isDraggingWindow || selectedWindowIndex < 0)
		return;

	// Apply movement to window polygon
	polybuilder.movePolygon(selectedWindowIndex, deltaX, deltaY);

	// Update clipped polygons (based on current active clipping algorithm)
	// Check if Sutherland-Hodgman clipped polygons exist
	bool hasShClipped = false;
	for (const auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::CLIPPED_SUTHERLAND_HODGMAN)
		{
			hasShClipped = true;
			break;
		}
	}

	// Check if Cyrus-Beck clipped polygons exist
	bool hasCbClipped = false;
	for (const auto& poly : polybuilder.getFinishedPolygons())
	{
		if (poly.type == PolyType::CLIPPED_CYRUS_BECK)
		{
			hasCbClipped = true;
			break;
		}
	}

	// Re-perform the active clipping algorithm(s)
	if (hasShClipped)
		performSutherlandHodgmanClipping(polybuilder);

	if (hasCbClipped)
		performCyrusBeckClipping(polybuilder);
}

void GUI::handleVertexDrag(GLFWwindow* window, PolyBuilder& polybuilder, float deltaX, float deltaY)
{
	// If we're not dragging, do nothing
	if (!isDraggingVertex || selectedShapeIndex < 0 || selectedVertexIndex < 0)
		return;

	polybuilder.updateVertexPosition(selectedShapeIndex, selectedVertexIndex, isShapePolygon, deltaX, deltaY);
}

bool GUI::startWindowDrag(GLFWwindow* window, int mouseButton, PolyBuilder& polybuilder)
{
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

	auto finishedPolys = polybuilder.getFinishedPolygons();

	// Find a window polygon that contains the mouse position
	for (size_t i = 0; i < finishedPolys.size(); i++)
	{
		const auto& poly = finishedPolys[i];
		if (poly.type == PolyType::WINDOW)
		{
			// Simple point-in-polygon test (bounding box for now, can be enhanced)
			const auto& verts = poly.getVertices();
			if (verts.size() > 0)
			{
				// Create a bounding box
				float minX = verts[0].x;
				float maxX = verts[0].x;
				float minY = verts[0].y;
				float maxY = verts[0].y;

				for (const auto& vert : verts)
				{
					minX = std::min(minX, vert.x);
					maxX = std::max(maxX, vert.x);
					minY = std::min(minY, vert.y);
					maxY = std::max(maxY, vert.y);
				}

				// Check if point is in bounding box
				if (ndcX >= minX && ndcX <= maxX && ndcY >= minY && ndcY <= maxY)
				{
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

void GUI::endDrag()
{
	isDraggingWindow = false;
	selectedWindowIndex = -1;
	isDraggingVertex = false;
	selectedShapeIndex = -1;
	selectedVertexIndex = -1;
}

bool GUI::tryStartVertexDrag(GLFWwindow* window, PolyBuilder polybuilder, double xPos, double yPos)
{
	int displayW, displayH;
	glfwGetFramebufferSize(window, &displayW, &displayH);

	// Convert mouse pos to NDC
	float ndcX = (2.0f * xPos) / displayW - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos) / displayH;

	auto finishedPolys = polybuilder.getFinishedPolygons();	

	// Check proximity to vertices
	const float hoverRadius = 0.02f;
	bool foundMatch = false;

	for (size_t i = 0; i < finishedPolys.size(); i++)
	{
		Polygon poly = finishedPolys[i];
		auto vertices = poly.getVertices();
		

		for (size_t j = 0; j < vertices.size(); j++)
		{
			Vertex vert = vertices[j];
			float dx = vert.x - ndcX;
			float dy = vert.y - ndcY;
			if (dx * dx + dy * dy < hoverRadius * hoverRadius)
			{
				selectedShapeIndex = i;
				selectedVertexIndex = j;
				foundMatch = true;
				isShapePolygon = true;
				isDraggingVertex = true;
				break;
			}
		}

		if (foundMatch) break;
	}

	if (foundMatch)
		return true;

	auto finishedBeziers = polybuilder.getFinishedBeziers();

	for (size_t i = 0; i < finishedBeziers.size(); i++)
	{
		Bezier bezier = finishedBeziers[i];
		auto controlPoints = bezier.getControlPoints();
		bool foundMatch = false;

		for (size_t j = 0; j < controlPoints.size(); j++)
		{
			Vertex vert = controlPoints[j];
			float dx = vert.x - ndcX;
			float dy = vert.y - ndcY;
			if (dx * dx + dy * dy < hoverRadius * hoverRadius)
			{
				selectedShapeIndex = i;
				selectedVertexIndex = j;
				foundMatch = true;
				isShapePolygon = false;
				isDraggingVertex = true;
				break;
			}
		}

		if (foundMatch) break;
	}

	if (foundMatch)
		return true;

	return false;
}

void GUI::handleFillClick(GLFWwindow* window, PolyBuilder& polyBuilder, double xPos, double yPos)
{
	if (!awaitingFillSeed)
		return;

	// Convert window coordinates to NDC
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float ndcX = (2.0f * xPos / width) - 1.0f;
	float ndcY = 1.0f - (2.0f * yPos / height);

	// First check: are we selecting a polygon?
	if (selectedPolygonIndex == -1)
	{
		// Find a polygon that contains this point
		const auto& polygons = polyBuilder.getFinishedPolygons();

		for (int i = 0; i < polygons.size(); i++)
		{
			const auto& poly = polygons[i];
			if (poly.type == PolyType::POLYGON)
			{
				// Simple point-in-polygon test
				const auto& vertices = poly.getVertices();

				// Check if point is close to any vertex
				for (const auto& vert : vertices)
				{
					float dx = vert.x - ndcX;
					float dy = vert.y - ndcY;
					if (dx * dx + dy * dy < 0.1f)
					{  // Selection radius
						selectedPolygonIndex = i;
						std::cout << "Selected polygon at index " << i << " for filling" << std::endl;

						// If not using seed fill, fill immediately
						if (Filler::getSelectedAlgorithm() != Filler::FILL_SEED)
							handleNonSeedFill(polyBuilder);

						return;
					}
				}
			}
		}
	}
	else if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED ||
		Filler::getSelectedAlgorithm() == Filler::FILL_SEED_RECURSIVE)
	{
		// We have a selected polygon index and we're using seed fill
		handleSeedFill(polyBuilder, ndcX, ndcY);
	}
}

// Helper method for non-seed fill algorithms
void GUI::handleNonSeedFill(PolyBuilder& polyBuilder)
{
	if (!polyBuilder.isValidPolygonIndex(selectedPolygonIndex))
		return;

	Polygon& selectedPolygon = polyBuilder.getPolygonAt(selectedPolygonIndex);
	std::vector<Vertex> fillPoints;

	if (Filler::getSelectedAlgorithm() == Filler::FILL_SCANLINE)
		fillPoints = Filler::fillPolygon(selectedPolygon);
	else
		fillPoints = Filler::fillPolygonLCA(selectedPolygon);

	// Get fill color
	float r, g, b, a;
	Filler::getFillColor(r, g, b, a);

	// Store the filled polygon
	polyBuilder.addFilledPolygon(selectedPolygon, fillPoints, r, g, b, a);

	// Reset state
	selectedPolygonIndex = -1;
	awaitingFillSeed = false;
}

// Helper method for seed fill algorithms
void GUI::handleSeedFill(PolyBuilder& polyBuilder, float ndcX, float ndcY)
{
	if (!polyBuilder.isValidPolygonIndex(selectedPolygonIndex))
		return;

	Polygon& selectedPolygon = polyBuilder.getPolygonAt(selectedPolygonIndex);
	std::vector<Vertex> fillPoints;

	if (Filler::getSelectedAlgorithm() == Filler::FILL_SEED)
		fillPoints = Filler::fillFromSeed(selectedPolygon, ndcX, ndcY);
	else
		fillPoints = Filler::fillFromSeedRecursive(selectedPolygon, ndcX, ndcY);

	// Get fill color
	float r, g, b, a;
	Filler::getFillColor(r, g, b, a);

	// Store the filled polygon
	polyBuilder.addFilledPolygon(selectedPolygon, fillPoints, r, g, b, a);

	// Reset state
	selectedPolygonIndex = -1;
	awaitingFillSeed = false;
}

void GUI::drawBuildingHelpTextbox(GLFWwindow* window, bool* open) // Added GLFWwindow if needed for viewport info, optional 'open' flag
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; // Top-left corner of the work area
	ImVec2 work_size = viewport->WorkSize; // Size of the work area

	// Define the properties of our anchored textbox window
	float window_width = 350.0f;    // Adjust as needed
	float window_padding = 10.0f;   // Padding from the very edge of the viewport

	// --- Position the window ---
	// We want to align the window's bottom-right corner with the viewport's bottom-right corner.
	ImVec2 window_pivot = ImVec2(1.0f, 1.0f); // Pivot at the window's bottom-right
	ImVec2 desired_window_pos = ImVec2(work_pos.x + work_size.x - window_padding,
		work_pos.y + work_size.y - window_padding);

	ImGui::SetNextWindowPos(desired_window_pos, ImGuiCond_Always, window_pivot);

	// --- Size the window ---
	// Set a fixed width and let the height auto-adjust to content.
	// If you want a multi-line input, you might set a fixed height too.
	ImGui::SetNextWindowSize(ImVec2(window_width, 0.0f), ImGuiCond_Always);

	// --- Window Flags ---
	// Minimal flags for a simple input box area
	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings | // Good for elements you always want in a fixed place
		ImGuiWindowFlags_AlwaysAutoResize; // Let height fit the content

	// --- Begin Window ---
	if (ImGui::Begin("Help", open, window_flags)) // 'open' is optional
	{
		ImGui::TextWrapped("Building a shape. Press space to finalize or 'C' to cancel.");
	}

	ImGui::End();
}