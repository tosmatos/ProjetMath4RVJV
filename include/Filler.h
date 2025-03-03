#pragma once
#include "Polygon.h"
#include <vector>

struct Edge {
	float x;        // Current x-coordinate of the edge
	float dx;       // Change in x for each y (inverse of slope)
	int yMax;       // Maximum y-coordinate for this edge

	Edge(float x, float dx, int yMax) : x(x), dx(dx), yMax(yMax) {}
};

class Filler {
private:
	// Screen dimensions for conversion between screen and NDC coordinates
	static int screenWidth;
	static int screenHeight;

	// Fill color
	static float fillColorR, fillColorG, fillColorB, fillColorA;

	// Array to track filled pixels
	static std::vector<std::vector<bool>> filledPixels;

	// Selected fill algorithm
	static int selectedAlgorithm; // 0 = Simple Scanline, 1 = LCA, 2 = Seed Fill

	// Convert between NDC (-1 to 1) and screen coordinates
	static Vertex NDCToScreen(const Vertex& ndcVertex);
	static Vertex ScreenToNDC(float x, float y);

	// Generate edge table for the polygon
	static std::vector<std::vector<Edge>> buildEdgeTable(const Polygon& polygon);

	// Add pixels for a horizontal scan line between x1 and x2 at y to the fill points   
	static void addScanLine(float x1, float x2, int y, std::vector<Vertex>& fillPoints);

public:
	// Fill algorithm constants
	static const int FILL_SCANLINE = 0;
	static const int FILL_LCA = 1;
	static const int FILL_SEED = 2;
	static const int FILL_SEED_RECURSIVE = 3;

	// Get/set the selected fill algorithm
	static int getSelectedAlgorithm() { return selectedAlgorithm; }
	static void setSelectedAlgorithm(int algorithm) { selectedAlgorithm = algorithm; }

	// Initialize the filler with screen dimensions
	static void init(int width, int height);

	// Set the fill color
	static void setFillColor(float r, float g, float b, float a);

	// Get the current fill color
	static void getFillColor(float& r, float& g, float& b, float& a) {
		r = fillColorR; g = fillColorG; b = fillColorB; a = fillColorA;
	}

	// Fill a polygon using the scanline algorithm
	// AKA "Lignes de balayge avec piles"
	static std::vector<Vertex> fillPolygon(const Polygon& polygon);

	// Fill a polygon using the LCA algorithm (Liste des Côtés Actifs)
	static std::vector<Vertex> fillPolygonLCA(const Polygon& polygon);

	// Seed-based filling (stack)
	// AKA "Algorithme à germes version piles"
	static std::vector<Vertex> fillFromSeed(const Polygon& polygon, float seedX, float seedY);

	// Recursive seed-based filling
	// AKA "Algorithme a germes version recursive"
	static std::vector<Vertex> fillFromSeedRecursive(const Polygon& polygon, float seedX, float seedY);
};