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
	void addVertex(Vertex vertex);
	void updateBuffers();
	const void draw() const;
	void drawPoints() const;
	const std::vector<Vertex>& getVertices() const;
	void setVertices(std::vector<Vertex> vertexVector);
	bool isClockwise() const;
	void reverseOrientation(); // Makes polygon clockwise if counter clockwise and the opposite
};