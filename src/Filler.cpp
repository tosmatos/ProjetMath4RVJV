#include "Filler.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <stack>
#include <GLFW/glfw3.h>

// Initialize static members
int Filler::screenWidth = 800;
int Filler::screenHeight = 600;
float Filler::fillColorR = 0.0f;
float Filler::fillColorG = 0.0f;
float Filler::fillColorB = 1.0f;
float Filler::fillColorA = 1.0f;
int Filler::selectedAlgorithm = Filler::FILL_SCANLINE;
std::vector<std::vector<bool>> Filler::filledPixels;

void Filler::init(int width, int height) {
	screenWidth = width;
	screenHeight = height;

	// Initialize the filled pixels array
	filledPixels.resize(height, std::vector<bool>(width, false));
}

void Filler::setFillColor(float r, float g, float b, float a) {
	fillColorR = r;
	fillColorG = g;
	fillColorB = b;
	fillColorA = a;
}

Vertex Filler::NDCToScreen(const Vertex& ndcVertex) {
	// Convert from NDC [-1,1] to screen coordinates [0,width/height]
	float screenX = (ndcVertex.x + 1.0f) * screenWidth / 2.0f;
	float screenY = (1.0f - ndcVertex.y) * screenHeight / 2.0f; // Y is flipped
	return Vertex(screenX, screenY);
}

Vertex Filler::ScreenToNDC(float x, float y) {
	// Convert from screen coordinates [0,width/height] to NDC [-1,1]
	float ndcX = (2.0f * x / screenWidth) - 1.0f;
	float ndcY = 1.0f - (2.0f * y / screenHeight); // Y is flipped
	return Vertex(ndcX, ndcY);
}

std::vector<std::vector<Edge>> Filler::buildEdgeTable(const Polygon& polygon) {
	const auto& vertices = polygon.getVertices();
	if (vertices.size() < 3) {
		return {};
	}

	// Convert vertices to screen coordinates
	std::vector<Vertex> screenVertices;
	for (const auto& v : vertices) {
		screenVertices.push_back(NDCToScreen(v));
	}

	// Find y-range of the polygon
	int yMin = screenHeight;
	int yMax = 0;

	for (const auto& v : screenVertices) {
		yMin = std::min(yMin, static_cast<int>(v.y));
		yMax = std::max(yMax, static_cast<int>(v.y));
	}

	// Clip to screen boundaries
	yMin = std::max(0, yMin);
	yMax = std::min(screenHeight - 1, yMax);

	// Create edge table
	std::vector<std::vector<Edge>> edgeTable(yMax - yMin + 1);

	for (size_t i = 0; i < screenVertices.size(); i++) {
		size_t j = (i + 1) % screenVertices.size();

		// Get the vertices of the edge
		Vertex v1 = screenVertices[i];
		Vertex v2 = screenVertices[j];

		// Skip horizontal edges
		if (static_cast<int>(v1.y) == static_cast<int>(v2.y)) {
			continue;
		}

		// Ensure v1 is the lower vertex
		if (v1.y > v2.y) {
			std::swap(v1, v2);
		}

		// Calculate edge parameters
		float dx = (v2.x - v1.x) / (v2.y - v1.y); // Inverse slope
		int yStart = static_cast<int>(std::ceil(v1.y));
		int y_end = static_cast<int>(std::ceil(v2.y));

		// Adjust starting x-coordinate for the first scan line
		float x = v1.x + dx * (yStart - v1.y);

		// Add edge to edge table
		if (yStart <= yMax && yStart >= yMin) {
			edgeTable[yStart - yMin].emplace_back(x, dx, y_end);
		}
	}

	return edgeTable;
}

void Filler::addScanLine(float x1, float x2, int y, std::vector<Vertex>& fillPoints) {
	if (x1 > x2) {
		std::swap(x1, x2);
	}

	// Clip to screen boundaries
	x1 = std::max(0.0f, std::min(static_cast<float>(screenWidth - 1), x1));
	x2 = std::max(0.0f, std::min(static_cast<float>(screenWidth - 1), x2));

	int startX = static_cast<int>(std::ceil(x1));
	int endX = static_cast<int>(std::floor(x2));

	for (int x = startX; x <= endX; x++) {
		// Mark pixel as filled
		if (y >= 0 && y < screenHeight && x >= 0 && x < screenWidth) {
			filledPixels[y][x] = true;

			// Convert to NDC and add to fill points
			Vertex ndcVertex = ScreenToNDC(static_cast<float>(x), static_cast<float>(y));
			fillPoints.push_back(ndcVertex);
		}
	}
}

std::vector<Vertex> Filler::fillPolygon(const Polygon& polygon) {
	std::vector<Vertex> fillPoints;

	// Convert vertices to screen coordinates
	const auto& vertices = polygon.getVertices();
	std::vector<Vertex> screenVertices;
	for (const auto& v : vertices) {
		screenVertices.push_back(NDCToScreen(v));
	}

	// Find y-range
	int yMin = screenHeight;
	int yMax = 0;

	for (const auto& v : screenVertices) {
		yMin = std::min(yMin, static_cast<int>(v.y));
		yMax = std::max(yMax, static_cast<int>(v.y));
	}

	// Clip to screen boundaries
	yMin = std::max(0, yMin);
	yMax = std::min(screenHeight - 1, yMax);

	// Get edge table
	auto edgeTable = buildEdgeTable(polygon);

	// Early exit if edge table is empty
	if (edgeTable.empty()) {
		return fillPoints;
	}

	// Active Edge List (AEL)
	std::vector<Edge> activeEdges;

	// Process each scan line
	for (int y = yMin; y <= yMax; y++) {
		int tableIndex = y - yMin;

		// Add new edges to AEL if we're within the edge table's range
		if (tableIndex >= 0 && tableIndex < edgeTable.size()) {
			activeEdges.insert(activeEdges.end(), edgeTable[tableIndex].begin(), edgeTable[tableIndex].end());
		}

		// Remove completed edges
		activeEdges.erase(
			std::remove_if(activeEdges.begin(), activeEdges.end(),
				[y](const Edge& e) { return y >= e.yMax; }),
			activeEdges.end()
		);

		// Sort active edges by x-coordinate
		std::sort(activeEdges.begin(), activeEdges.end(),
			[](const Edge& a, const Edge& b) { return a.x < b.x; });

		// Fill scan line segments - pairs of intersections
		for (size_t i = 0; i < activeEdges.size(); i += 2) {
			if (i + 1 < activeEdges.size()) {
				addScanLine(activeEdges[i].x, activeEdges[i + 1].x, y, fillPoints);
			}
		}

		// Update x-coordinates for the next scan line
		for (auto& edge : activeEdges) {
			edge.x += edge.dx;
		}
	}

	std::cout << "Filled polygon with " << fillPoints.size() << " points" << std::endl;
	return fillPoints;
}

std::vector<Vertex> Filler::fillPolygonLCA(const Polygon& polygon) {
	// This is the main LCA (List of Active Edges) algorithm
	// It's similar to fillPolygon but follows the algorithm described in your course materials

	std::vector<Vertex> fillPoints;

	// Convert vertices to screen coordinates
	const auto& vertices = polygon.getVertices();
	std::vector<Vertex> screenVertices;
	for (const auto& v : vertices) {
		screenVertices.push_back(NDCToScreen(v));
	}

	// Find y range
	int yMin = screenHeight;
	int yMax = 0;

	for (const auto& v : screenVertices) {
		yMin = std::min(yMin, static_cast<int>(v.y));
		yMax = std::max(yMax, static_cast<int>(v.y));
	}

	// Clip to screen boundaries
	yMin = std::max(0, yMin);
	yMax = std::min(screenHeight - 1, yMax);

	// Intermediate structure (SI) - an array of edge lists indexed by y-coordinate
	std::vector<std::vector<Edge>> si(yMax - yMin + 1);

	// Build the SI structure
	for (size_t i = 0; i < screenVertices.size(); i++) {
		size_t j = (i + 1) % screenVertices.size();

		// Get the vertices of the edge
		Vertex v1 = screenVertices[i];
		Vertex v2 = screenVertices[j];

		// Skip horizontal edges
		if (static_cast<int>(v1.y) == static_cast<int>(v2.y)) {
			continue;
		}

		// Ensure v1 is the lower vertex
		if (v1.y > v2.y) {
			std::swap(v1, v2);
		}

		// Calculate edge parameters
		float dx = (v2.x - v1.x) / (v2.y - v1.y); // Inverse slope
		int yStart = static_cast<int>(std::ceil(v1.y));
		int yEnd = static_cast<int>(std::ceil(v2.y));

		// Adjust starting x-coordinate for the first scan line
		float x = v1.x + dx * (yStart - v1.y);

		// Add edge to SI
		if (yStart <= yMax && yStart >= yMin) {
			si[yStart - yMin].emplace_back(x, dx, yEnd);
		}
	}

	// Active Edge List (AEL)
	std::vector<Edge> ael;

	// Process each scan line
	for (int y = yMin; y <= yMax; y++) {
		int index = y - yMin;

		// Add new edges from SI to AEL
		if (index >= 0 && index < si.size()) {
			ael.insert(ael.end(), si[index].begin(), si[index].end());
		}

		// Remove completed edges
		ael.erase(
			std::remove_if(ael.begin(), ael.end(),
				[y](const Edge& e) { return y >= e.yMax; }),
			ael.end()
		);

		// Sort active edges by x-coordinate
		std::sort(ael.begin(), ael.end(),
			[](const Edge& a, const Edge& b) { return a.x < b.x; });

		// Fill scan line segments
		for (size_t i = 0; i < ael.size(); i += 2) {
			if (i + 1 < ael.size()) {
				addScanLine(ael[i].x, ael[i + 1].x, y, fillPoints);
			}
		}

		// Update x-coordinates for the next scan line
		for (auto& edge : ael) {
			edge.x += edge.dx;
		}
	}

	std::cout << "Filled polygon with LCA algorithm, " << fillPoints.size() << " points" << std::endl;
	return fillPoints;
}

// Seed fill implementation
std::vector<Vertex> Filler::fillFromSeed(const Polygon& polygon, float seedX, float seedY) {
	std::vector<Vertex> fillPoints;

	// Convert seed coordinates to screen space
	Vertex seed = NDCToScreen(Vertex(seedX, seedY));
	int seedScreenX = static_cast<int>(seed.x);
	int seedScreenY = static_cast<int>(seed.y);

	// Check if seed is within screen bounds
	if (seedScreenX < 0 || seedScreenX >= screenWidth ||
		seedScreenY < 0 || seedScreenY >= screenHeight) {
		std::cerr << "Seed point is outside screen bounds" << std::endl;
		return fillPoints;
	}

	// Create a buffer for the border pixels
	std::vector<std::vector<bool>> borderPixels(screenHeight, std::vector<bool>(screenWidth, false));

	const auto& vertices = polygon.getVertices();
	for (size_t i = 0; i < vertices.size(); i++) {
		size_t j = (i + 1) % vertices.size();

		Vertex v1 = NDCToScreen(vertices[i]);
		Vertex v2 = NDCToScreen(vertices[j]);

		// Simple line drawing algorithm (Bresenham's) to mark border pixels
		int x1 = static_cast<int>(std::round(v1.x));
		int y1 = static_cast<int>(std::round(v1.y));
		int x2 = static_cast<int>(std::round(v2.x));
		int y2 = static_cast<int>(std::round(v2.y));

		int dx = std::abs(x2 - x1);
		int dy = std::abs(y2 - y1);
		int sx = (x1 < x2) ? 1 : -1;
		int sy = (y1 < y2) ? 1 : -1;
		int err = dx - dy;

		while (true) {
			// Mark current pixel and adjacent pixels for a 2-pixel wide border
			for (int offsetY = -1; offsetY <= 1; offsetY++) {
				for (int offsetX = -1; offsetX <= 1; offsetX++) {
					int px = x1 + offsetX;
					int py = y1 + offsetY;

					if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
						borderPixels[py][px] = true;
					}
				}
			}

			if (x1 == x2 && y1 == y2) break;

			int e2 = 2 * err;
			if (e2 > -dy) {
				err -= dy;
				x1 += sx;
			}
			if (e2 < dx) {
				err += dx;
				y1 += sy;
			}
		}
	}

	// Reset filled pixels
	filledPixels = std::vector<std::vector<bool>>(screenHeight, std::vector<bool>(screenWidth, false));

	// Stack for the seed fill algorithm
	std::stack<std::pair<int, int>> pixelStack;
	pixelStack.push({ seedScreenX, seedScreenY });

	// Process pixels until stack is empty
	while (!pixelStack.empty()) {
		auto [x, y] = pixelStack.top();
		pixelStack.pop();

		// Skip if out of bounds, already filled, or on border
		if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight ||
			filledPixels[y][x] || borderPixels[y][x]) {
			continue;
		}

		// Mark pixel as filled
		filledPixels[y][x] = true;

		// Add to the vertices to be drawn
		Vertex ndcVertex = ScreenToNDC(static_cast<float>(x), static_cast<float>(y));
		fillPoints.push_back(ndcVertex);

		// Add neighboring pixels to stack
		pixelStack.push({ x + 1, y }); // right
		pixelStack.push({ x - 1, y }); // left
		pixelStack.push({ x, y + 1 }); // up
		pixelStack.push({ x, y - 1 }); // down
	}

	std::cout << "Filled from seed with " << fillPoints.size() << " points" << std::endl;
	return fillPoints;
}