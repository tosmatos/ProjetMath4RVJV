#include "PolyBuilder.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <iostream>
#include <string>

namespace PolyBuilder
{
	Type polyType;
	bool buildingPoly;
	Polygon tempPolygon; // The one for the building process
	Polygon polygon;
	Polygon window;
	std::vector<Polygon> finishedPolygons;
	std::vector<FilledPolygon> filledPolygons;

	void StartPolygon(Type type)
	{
		polyType = type;
		buildingPoly = true;
		tempPolygon = Polygon();
	}

	void AppendVertex(double xPos, double yPos)
	{
		if (!buildingPoly)
			return;

		// Get window size
		int width, height;
		glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

		// Convert to normalized device coordinates (-1 to 1)
		float normalizedX = (2.0f * xPos / width) - 1.0f;
		float normalizedY = 1.0f - (2.0f * yPos / height);  // Flip Y coordinate

		std::cout << "Added a vertex at X : " << std::to_string(normalizedX) << ", Y : " << std::to_string(normalizedY) << std::endl;

		tempPolygon.addVertex(normalizedX, normalizedY);
	}

	void Finish()
	{
		std::cout << "Finishing polygon ..." << std::endl;

		if (!buildingPoly)
			return;

		std::cout << "Polygon is " << (tempPolygon.isClockwise() ? "clockwise" : "counter-clockwise.") << std::endl;

		switch (polyType)
		{
		case (POLYGON):
			polygon = tempPolygon;
			polygon.type = POLYGON;
			polygon.updateBuffers();
			finishedPolygons.push_back(polygon);
			break;

		case (WINDOW):
			window = tempPolygon;
			window.type = WINDOW;
			window.updateBuffers();
			finishedPolygons.push_back(window);
			break;
		}
		
		buildingPoly = false;
		tempPolygon = Polygon();
	}

	void Cancel()
	{
		buildingPoly = false;
		tempPolygon = Polygon();
	}


	void MovePolygon(int polyIndex, float deltaX, float deltaY)
	{
		// Check for valid index
		if (polyIndex < 0 || polyIndex >= finishedPolygons.size())
			return;

		// Get a reference to the polygon to modify
		Polygon& poly = finishedPolygons[polyIndex];

		// Get a modifiable copy of the vertices
		std::vector<Vertex> modifiedVertices = poly.getVertices();

		// Apply translation to each vertex
		for (auto& vertex : modifiedVertices) {
			vertex.x += deltaX;
			vertex.y += deltaY;
		}

		// Update the polygon with the new vertices
		poly.setVertices(modifiedVertices);

		// Re-initialize the OpenGL buffers
		poly.updateBuffers();
	}
	
	// Add a filled polygon to our storage
	void AddFilledPolygon(const Polygon& poly,
		const std::vector<Vertex>& fillPoints,
		float r, float g, float b, float a)
	{
		FilledPolygon filled;
		filled.polygon = poly;
		filled.fillPoints = fillPoints;
		filled.colorR = r;
		filled.colorG = g;
		filled.colorB = b;
		filled.colorA = a;

		// Create OpenGL buffers for the fill points
		glGenVertexArrays(1, &filled.vao);
		glGenBuffers(1, &filled.vbo);

		// Upload fill points to GPU
		glBindVertexArray(filled.vao);
		glBindBuffer(GL_ARRAY_BUFFER, filled.vbo);
		glBufferData(GL_ARRAY_BUFFER,
			fillPoints.size() * sizeof(Vertex),
			fillPoints.data(),
			GL_STATIC_DRAW);

		// Set up vertex attributes
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		// Add to our collection
		filledPolygons.push_back(filled);
	}

	// Clear all filled polygons
	void ClearFilledPolygons()
	{
		for (auto& filled : filledPolygons) {
			// Delete OpenGL resources
			if (filled.vao != 0) {
				glDeleteVertexArrays(1, &filled.vao);
				glDeleteBuffers(1, &filled.vbo);
			}
		}
		filledPolygons.clear();
	}
}