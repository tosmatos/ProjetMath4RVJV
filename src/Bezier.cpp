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
        glGenVertexArrays(1, &curveVAO);
        glGenBuffers(1, &controlVBO);
        glGenBuffers(1, &curveVBO);
        buffersInitialized = true;
    }

    // For more detailed info on those gl functions check out Polygon.cpp which has detailed comments
    // Step 1: Bind the VAO - this records all subsequent buffer settings
    glBindVertexArray(controlVAO);

    // Step 2: Bind the VBO and upload vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, controlVBO);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(Vertex), controlPoints.data(), GL_STATIC_DRAW);

    // Step 3: Tell OpenGL how to interpret our vertex data
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);f

    // Now do the same thing for the generated curve vertices
    glBindVertexArray(curveVAO);

    glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
    glBufferData(GL_ARRAY_BUFFER, generatedCurve.size() * sizeof(Vertex), generatedCurve.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Enable the vertex attribute we just configured
    glEnableVertexAttribArray(0);
}

const void Bezier::drawControlPoints(Shader& shader) const
{
    shader.Use();
    shader.SetColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f);

    // Debug: check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "OpenGL error before drawing: " << error << std::endl;
    }

    glBindVertexArray(controlVAO);
    glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());
    glDrawArrays(GL_POINTS, 0, controlPoints.size());

    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "OpenGL error after drawing: " << error << std::endl;
    }
}

const void Bezier::drawGeneratedCurve(Shader& shader) const
{
    shader.Use();
    // Maybe that can be where we set shader parameters like color
    glBindVertexArray(curveVAO);
    glDrawArrays(GL_LINE_STRIP, 0, generatedCurve.size());
}

void Bezier::generateCurve()
{
    if (controlPoints.size() < 2)
    {
        std::cout << "Not enough control points to generated a bézier curve." << std::endl;
        return;
    }

    generatedCurve.clear(); // Clear previous points

    // TODO : Figure out how to implement these algorithms.

    if (algorithm == 0)
        generatePascalCurve();
    else if (algorithm == 1)
        generateDeCasteljauCurve();
}

void Bezier::generatePascalCurve()
{
    // TODO : Implement that algorithm !
}

void Bezier::generateDeCasteljauCurve()
{
    // TODO : Implement that algorithm !
}

void Bezier::setAlgorithm(int algo)
{
    if (algo != 0 || algo != 1)
    {
        std::cout << "Can't set Bézier algorithm to something other than 0 or 1. Value passed : " << algo << std::endl;
        return;
    }

    algorithm = algo;
}
