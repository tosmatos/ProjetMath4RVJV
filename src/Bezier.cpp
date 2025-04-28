#include "Bezier.h"

#include <algorithm>
#include <glad/glad.h>
#include <iostream>

Bezier::Bezier()
{

}

Bezier::Bezier(const Bezier& other) : controlPoints(other.controlPoints), buffersInitialized(false)
{ // Copy constructor
	controlPoints = other.controlPoints;
	
	//generatedCurve = other.generatedCurve; // Will I need this how do I recompute it every time ?
	
	//type = other.type;
	if (other.buffersInitialized)
	{
		updateBuffers();
	}
}

Bezier& Bezier::operator=(const Bezier& other)
{ // Copy operator
	if (this != &other)
	{
		if (buffersInitialized)
		{
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			buffersInitialized = false;
		}
		controlPoints = other.controlPoints;
		//generatedCurve = other.generatedCurve; // Will I need this how do I recompute it every time ?
		
		//type = other.type;
		if (other.buffersInitialized)
		{
			updateBuffers();
		}
	}
	return *this;
}

Bezier::~Bezier()
{
	if (buffersInitialized)
	{
		// When polygon is destroyed, clean up the OpenGL buffers
		// This prevents memory leaks in the GPU
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
}

void Bezier::addControlPoint(float x, float y)
{
	controlPoints.push_back(Vertex(x, y));
}

void Bezier::addControlPoint(Vertex vertex)
{
	controlPoints.push_back(vertex);
}

void Bezier::updateBuffers()
{
	if (!buffersInitialized)
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		buffersInitialized = true;
	}

	// Step 1: Bind the VAO - this records all subsequent buffer settings
	glBindVertexArray(VAO);

	// Step 2: Bind the VBO and upload vertex data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER,                // Target buffer
		controlPoints.size() * sizeof(Vertex),// Size of data in bytes
		controlPoints.data(),                 // Pointer to our vertex data
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