#include "Bezier.h"

#include <algorithm>
#include <glad/glad.h>
#include <iostream>
#include <cmath> // For pow()
#include <chrono> // For calculating generation time
#include <iomanip> // For number of digits when printing time

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
            glDeleteVertexArrays(1, &curveVAO);
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
        glDeleteVertexArrays(1, &curveVAO);
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

    // Debug: check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "OpenGL error before drawing: " << error << std::endl;
    }

    glBindVertexArray(controlVAO);

    shader.setColor("uColor", 1.0f, 0.0f, 0.5f, 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
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

    glBindVertexArray(curveVAO);

    shader.setColor("uColor", 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, generatedCurve.size());
}

const void Bezier::drawControlPointsPreview(Shader& shader) const
{
    shader.use();

    glBindVertexArray(controlVAO);

    shader.setColor("uColor", 1.0f, 1.0f, 0.0f, 0.5f);
    glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, controlPoints.size());

}

const void Bezier::drawGeneratedCurvePreview(Shader& shader) const
{
    shader.use();

    glBindVertexArray(curveVAO);

    shader.setColor("uColor", 0.0f, 1.0f, 1.0f, 1.0f);
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

    auto start = std::chrono::steady_clock::now(); // type is time_point

    // Then call appropriate function
    if (algorithm == 0)
        generatePascalCurve();
    else if (algorithm == 1)
        generateDeCasteljauCurve();

    auto end = std::chrono::steady_clock::now();
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    generationTime = time_span.count();

    //std::cout << "Generation took " << std::fixed << std::setprecision(7) << generationTime << " seconds." << std::endl;
}

void Bezier::generatePascalCurve()
{
    //std::cout << "Generating curve with Pascal algorithm..." << std::endl;
    auto start = std::chrono::steady_clock::now(); // type is time_point

    // the degree of the bézier curve
    int degree = controlPoints.size() - 1;

    // Compute number of segments beforehand
    // Because if it becomes too small, errors accumualte and the loop never finishes
    int numSegments = static_cast<int>(1.0f / stepSize);

    // t represents the position along the curve
    for (int i = 0; i <= numSegments; i++)
    {
        // We calculate t here
        float t = static_cast<float>(i) / numSegments;

        Vertex pointOnCurve; // Initialize the result vertex

        // sum of the contributions of each control point
        for (int j = 0; j <= degree; ++j)
        {
            // Calculate the Bernstein polynomial term for the j-th control point and current 't'
            // The formula is: BinomialCoefficient(n, j) * (1-t)^(n-j) * t^j
            // Where n is the degree of the curve
            // C.f Maths Chap 1 Courbes - 3 Base de bernstein
            float bernsteinTerm = static_cast<float>(combinations(degree, j)) *
                std::pow(1.0 - t, degree - j) *
                std::pow(t, j);

            // Add the weighted control point to the current point on the curve
            pointOnCurve += controlPoints[j] * bernsteinTerm;
        }

        generatedCurve.push_back(pointOnCurve);
    }

    // Update buffers to send the new curve data to the GPU
    updateBuffers();
}

// For now this takes longer than Pascal, and the reason is probably the vector creation and copying in the loop.
// New bonus point to do !
void Bezier::generateDeCasteljauCurve()
{
    //std::cout << "Generating curve with DeCasteljau algorithm..." << std::endl;

    // Compute number of segments beforehand
    // Because if it becomes too small, errors accumualte and the loop never finishes
    int numSegments = static_cast<int>(1.0f / stepSize);

    // t represents the position along the curve
    for (int i = 0; i <= numSegments; i++)
    {
        // We calculate t here
        float t = static_cast<float>(i) / numSegments;

        Vertex pointOnCurve;

        // Initialize the temp list with all points by default.
        // Will get one point smaller with each loop
        std::vector<Vertex> currentPoints = controlPoints;

        while (currentPoints.size() > 1)
        {
            std::vector<Vertex> newPoints;

            for (int j = 0; j < currentPoints.size() - 1; j++)
            {
                Vertex Pa = currentPoints[j];
                Vertex Pb = currentPoints[j + 1];
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