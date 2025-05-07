#include "Bezier.h"

#include <algorithm>
#include <glad/glad.h>
#include <iostream>
#include <cmath>

// Helper function to calculate binomial coefficient C(n, k)
// This is the mathematical representation of the numbers in Pascal's triangle.
// C(n, k) = n! / (k! * (n-k)!)
// See Maths Chap 1 Courbes - 3 Base de bernstein
long long static combinations(int n, int k)
{
    // Handle invalid inputs
    if (k < 0 || k > n)
    {
        return 0;
    }
    // C(n, 0) and C(n, n) are always 1
    if (k == 0 || k == n)
    {
        return 1;
    }
    // Use symmetry: C(n, k) = C(n, n-k) to reduce calculations
    // Choisir k éléments parmi n est la même chose que choisir de ne pas choisir n−k éléments parmi n.
    if (k > n / 2)
    {
        k = n - k;
    }
    // Calculate the binomial coefficient iteratively
    long long res = 1;
    for (int i = 1; i <= k; ++i)
    {
        res = res * (n - i + 1) / i;
    }
    return res;
}

Bezier::Bezier()
{

}

Bezier::Bezier(const Bezier& other) : controlPoints(other.controlPoints), buffersInitialized(false)
{ // Copy constructor
    controlPoints = other.controlPoints;
    
    generatedCurve = other.generatedCurve;
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
        generatedCurve = other.generatedCurve;
        
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
    glEnableVertexAttribArray(0);

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
    shader.use();
    shader.setColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f);

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
    shader.use();
    shader.setColor("uColor", 0.0f, 0.0f, 1.0f, 1.0f);

    glBindVertexArray(curveVAO);
    glDrawArrays(GL_LINE_STRIP, 0, generatedCurve.size());
}

void Bezier::generateCurve()
{
    if (controlPoints.size() < 2)
    {
        std::cout << "Not enough control points to generate a bézier curve." << std::endl;
        return;
    }

    generatedCurve.clear(); // Clear previous points

    // Then call appropriate function
    if (algorithm == 0)
        generatePascalCurve();
    else if (algorithm == 1)
        generateDeCasteljauCurve();
}

void Bezier::generatePascalCurve()
{
    std::cout << "Generating curve with DeCasteljau algorithm..." << std::endl;
    // the degree of the bézier curve
    int degree = controlPoints.size() - 1;

    // t represents the position along the curve
    for (float t = 0.0f; t <= 1.0f; t += stepSize)
    {
        Vertex pointOnCurve; // Initialize the result vertex

        // sum of the contributions of each control point
        for (int i = 0; i <= degree; ++i)
        {
            // Calculate the Bernstein polynomial term for the i-th control point and current 't'
            // The formula is: BinomialCoefficient(n, i) * (1-t)^(n-i) * t^i
            // Where n is the degree of the curve
            // C.f Maths Chap 1 Courbes - 3 Base de bernstein
            float bernsteinTerm = static_cast<float>(combinations(degree, i)) *
                std::pow(1.0 - t, degree - i) *
                std::pow(t, i);

            // Add the weighted control point to the current point on the curve
            pointOnCurve += controlPoints[i] * bernsteinTerm;
        }

        generatedCurve.push_back(pointOnCurve);
    }

    // Update buffers to send the new curve data to the GPU
    updateBuffers();
}

void Bezier::generateDeCasteljauCurve()
{
    std::cout << "Generating curve with DeCasteljau algorithm..." << std::endl;

    // t represents the position along the curve
    for (float t = 0.0f; t <= 1.0f; t += stepSize)
    {
        Vertex pointOnCurve;

        // Initialize the temp list with all points by default.
        // Will get one point smaller with each loop
        std::vector<Vertex> currentPoints = controlPoints;

        while (currentPoints.size() > 1)
        {
            std::vector<Vertex> newPoints;

            for (int i = 0; i < currentPoints.size() - 1; i++)
            {
                Vertex Pa = currentPoints[i];
                Vertex Pb = currentPoints[i + 1];
                Vertex Pab = Pa * (1.0f - t) + Pb * t; // (1−t)⋅* Pa​ + t *⋅Pb formule interpolation linéaire decasteljau
                newPoints.push_back(Pab);
            }

            currentPoints = newPoints;
        }

        // After that loop, there's only one point in currentPoints. 
        // That's the one we add to the generatedCurve vector
        generatedCurve.push_back(currentPoints[0]);
    }

    updateBuffers();
}