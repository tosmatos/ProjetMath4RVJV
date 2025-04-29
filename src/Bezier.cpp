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
			glDeleteVertexArrays(1, &controlVAO);
			glDeleteBuffers(1, &controlVBO);
			glDeleteBuffers(1, &curveVBO);
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
		glDeleteVertexArrays(1, &controlVAO);
		glDeleteBuffers(1, &controlVBO);
		glDeleteBuffers(1, &curveVBO);
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
		glGenVertexArrays(1, &controlVAO);
		glGenBuffers(1, &controlVBO);
		glGenBuffers(1, &curveVBO);
		buffersInitialized = true;
	}

	// Step 1: Bind the VAO - this records all subsequent buffer settings
	glBindVertexArray(controlVAO);

	// Step 2: Bind the VBO and upload vertex data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, controlVBO);
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

	// Now do the same thing for the generated curve vertices
	glBindVertexArray(curveVAO);

	glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
	glBufferData(GL_ARRAY_BUFFER, generatedCurve.size() * sizeof(Vertex), generatedCurve.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	// Enable the vertex attribute we just configured
	glEnableVertexAttribArray(0);
}