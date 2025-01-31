#pragma once

#include <vector>
#include "Vertex.h"

class Polygon
{
private:
	std::vector<Vertex> vertices; // Array of vertices - the polygon itself
	unsigned int VAO, VBO; // Vertex Array Object and Buffer Object for OpenGL

public:
	Polygon();
	~Polygon();
	void addVertex(float x, float y);
	void updateBuffers();
	void draw();
	const std::vector<Vertex>& getVertices() const;
};