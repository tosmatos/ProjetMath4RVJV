#include "Polygon.h"
#include <glad/glad.h>
#include <iostream>

// I've left Claude's explanation because I need to be reminded every time I look at the code
// This shit feels like black magic to me for now. 

Polygon::Polygon()
{
	// Create OpenGL buffer objects that we'll need for rendering
	// VAO (Vertex Array Object) stores the format of our vertex data
	// VBO (Vertex Buffer Object) stores the actual vertex data
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
}

Polygon::~Polygon()
{
	// When polygon is destroyed, clean up the OpenGL buffers
	// This prevents memory leaks in the GPU
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Polygon::addVertex(float x, float y)
{
	// Add a new vertex to our vector of vertices
	// Note: This only updates our CPU-side data
	// We need to call updateBuffers() to send this to the GPU
	vertices.push_back(Vertex(x, y));
}

void Polygon::updateBuffers()
{
	// Step 1: Bind the VAO - this records all subsequent buffer settings
	glBindVertexArray(VAO);

	// Step 2: Bind the VBO and upload vertex data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER,                // Target buffer
		vertices.size() * sizeof(Vertex),// Size of data in bytes
		vertices.data(),                 // Pointer to our vertex data
		GL_STATIC_DRAW);                 // Usage hint: data won't change much

	// Step 3: Tell OpenGL how to interpret our vertex data
	glVertexAttribPointer(
		0,                    // Attribute location (0 = position)
		2,                    // Size (2 components: x, y)
		GL_FLOAT,            // Data type of each component
		GL_FALSE,            // Should OpenGL normalize the data?
		sizeof(Vertex),      // Stride (bytes between consecutive vertices)
		(void*)0             // Offset of first component
	);
	// Enable the vertex attribute we just configured
	glEnableVertexAttribArray(0);
}

void Polygon::draw()
{
	// We need to bind the VAO again because:
	// 1. Other objects might have bound their VAO since our last draw
	// 2. OpenGL is a state machine - it needs to know which VAO to use
	glBindVertexArray(VAO);

	// Draw the polygon as a line loop
	// GL_LINE_LOOP connects all points with lines and closes the shape
	glDrawArrays(GL_LINE_LOOP,        // Drawing mode
		0,                    // Start vertex
		vertices.size());     // Number of vertices to draw
}

const std::vector<Vertex>& Polygon::getVertices() const
{
	// Provide read-only access to our vertices
	return vertices;
}