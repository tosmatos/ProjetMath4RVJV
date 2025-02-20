#pragma once

#include <vector>
#include "Vertex.h"
#include "PolyTypes.h"

class Polygon
{
private:
	std::vector<Vertex> vertices; // Array of vertices - the polygon itself
	unsigned int VAO, VBO; // Vertex Array Object and Buffer Object for OpenGL
	bool buffersInitialized = false;

public:
	PolyBuilder::Type type;

	Polygon();
	Polygon(const Polygon& other); // Copy constructor
	Polygon& operator=(const Polygon& other); // Assignment operator
	~Polygon();
	void addVertex(float x, float y);
	void updateBuffers();
	const void draw() const;
	void drawPoints() const;
	const std::vector<Vertex>& getVertices() const;
	bool isClockwise() const;
};