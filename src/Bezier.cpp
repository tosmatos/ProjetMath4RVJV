﻿#include "Bezier.h"
#include "MathUtils.h"

#include <algorithm>
#include <glad/glad.h>
#include <iostream>
#include <cmath> // For pow()
#include <chrono> // For calculating generation time
#include <iomanip> // For number of digits when printing time

using namespace MathUtils;

void Bezier::generatePascalCurve()
{
    //std::cout << "Generating curve with Pascal algorithm..." << std::endl;

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
}

// For now this takes longer than Pascal, and the reason is probably the vector creation and copying in the loop.
// New bonus point to do !
void Bezier::generateDeCasteljauCurve()
{
    // Calculate how many segments we'll use to approximate the curve
    int numberOfSegments = static_cast<int>(1.0f / stepSize);

    // Store the total number of control points
    int numberOfControlPoints = controlPoints.size();

    // Pre-allocate a single temporary array to hold intermediate points
    // This avoids repeated memory allocation inside the loops
    std::vector<Vertex> intermediatePoints(numberOfControlPoints);

    // For each point on the curve we want to generate
    for (int segmentIndex = 0; segmentIndex <= numberOfSegments; segmentIndex++)
    {
        // Calculate the parameter t (ranges from 0 to 1)
        float parameterT = static_cast<float>(segmentIndex) / numberOfSegments;

        // Initialize our working array with the original control points
        // We'll modify these values in-place during the algorithm
        for (int pointIndex = 0; pointIndex < numberOfControlPoints; pointIndex++) {
            intermediatePoints[pointIndex] = controlPoints[pointIndex];
        }

        // Apply De Casteljau's algorithm
        for (int level = 1; level < numberOfControlPoints; level++) {
            // For each pair of adjacent points at the current level
            for (int pointIndex = 0; pointIndex < numberOfControlPoints - level; pointIndex++) {
                // Perform linear interpolation between adjacent points
                Vertex startPoint = intermediatePoints[pointIndex];
                Vertex endPoint = intermediatePoints[pointIndex + 1];

                // The formula for linear interpolation: (1-t)*P0 + t*P1
                intermediatePoints[pointIndex] = startPoint * (1.0f - parameterT) +
                    endPoint * parameterT;
            }
        }

        // After all levels, the only element of intermediatePoints is the point on the curve
        // corresponding to the current parameter t
        generatedCurve.push_back(intermediatePoints[0]);
    }
}

Bezier::Bezier()
{

}

Bezier::Bezier(const Bezier& other) : controlPoints(other.controlPoints), buffersInitialized(false)
{ // Copy constructor
    controlPoints = other.controlPoints;
    
    generatedCurve = other.generatedCurve;
    convexHull = other.convexHull;
    generationTime = other.generationTime;
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
            glDeleteVertexArrays(1, &hullVAO);
            glDeleteBuffers(1, &controlVBO);
            glDeleteBuffers(1, &curveVBO);
            glDeleteBuffers(1, &hullVBO);
            buffersInitialized = false;
        }
        controlPoints = other.controlPoints;
        generatedCurve = other.generatedCurve;
        convexHull = other.convexHull;
        generationTime = other.generationTime;
        
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
        glDeleteVertexArrays(1, &hullVAO);
        glDeleteBuffers(1, &controlVBO);
        glDeleteBuffers(1, &curveVBO);
        glDeleteBuffers(1, &hullVAO);
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
        glGenVertexArrays(1, &hullVAO);
        glGenBuffers(1, &controlVBO);
        glGenBuffers(1, &curveVBO);
        glGenBuffers(1, &hullVBO);
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
    
    if (convexHull.size() != 0)
    {
        // Again for the hull
        glBindVertexArray(hullVAO);

        glBindBuffer(GL_ARRAY_BUFFER, hullVBO);
        glBufferData(GL_ARRAY_BUFFER, convexHull.size() * sizeof(Vertex), convexHull.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        // Enable the vertex attribute we just configured
        glEnableVertexAttribArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

const void Bezier::drawControlPoints(Shader& shader) const
{
    shader.use();

    // Debug: check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "drawControlPoints -- OpenGL error before drawing: " << error << std::endl;
    }

    glBindVertexArray(controlVAO);
    
    shader.setColor("uColor", 1.0f, 0.0f, 0.5f, 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, controlPoints.size());

    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "drawControlPoints --  OpenGL error after drawing: " << error << std::endl;
    }

    glBindVertexArray(0); // Unbind to prevent side effects
}

const void Bezier::drawGeneratedCurve(Shader& shader) const
{
    shader.use();

    glBindVertexArray(curveVAO);

    shader.setColor("uColor", 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, generatedCurve.size());

    glBindVertexArray(0); // Unbind to prevent side effects
}

const void Bezier::drawControlPointsPreview(Shader& shader) const
{
    shader.use();

    glBindVertexArray(controlVAO);

    shader.setColor("uColor", 1.0f, 1.0f, 0.0f, 0.5f);
    glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, controlPoints.size());

    glBindVertexArray(0); // Unbind to prevent side effects
}

const void Bezier::drawGeneratedCurvePreview(Shader& shader) const
{
    shader.use();

    glBindVertexArray(curveVAO);

    shader.setColor("uColor", 0.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINE_STRIP, 0, generatedCurve.size());

    glBindVertexArray(0); // Unbind to prevent side effects
}

const void Bezier::drawConvexHull(Shader& shader) const
{
    shader.use();
    glBindVertexArray(hullVAO);

    shader.setColor("uColor", 0.0f, 1.0f, 0.5f, 0.25f);
    glDrawArrays(GL_LINE_LOOP, 0, convexHull.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 0.25f);
    glDrawArrays(GL_POINTS, 0, convexHull.size());

    glBindVertexArray(0); // Unbind to prevent side effects
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

void Bezier::incrementStepSize()
{
    if (stepSize >= 1.0f)
        return;

    stepSize += 0.001f;
    generateCurve();
}

void Bezier::decrementStepSize()
{
    if (stepSize <= 0.001f)
        return;

    stepSize -= 0.001f;
    generateCurve();
}

void Bezier::duplicateControlPoint(int index)
{
    controlPoints.insert(controlPoints.begin() + index, controlPoints[index]);
    generateCurve();
}

// Jarvis March Algorithm https://en.wikipedia.org/wiki/Gift_wrapping_algorithm
void Bezier::generateConvexHull()
{
    int pointsSize = controlPoints.size();

    if (pointsSize <= 2)
        return;

    std::vector<Vertex> resultHull;    
    int leftmost = 0; // leftmost index

    // Find leftmost point. i = 1 cause point initialized to first point in list
    for (int i = 1; i < pointsSize; i++)
    {
        if (controlPoints[i].x < controlPoints[leftmost].x)
            leftmost = i;
    }

    int currentPoint = leftmost;
    int nextPoint;

    do
    {
        resultHull.push_back(controlPoints[currentPoint]);

        // % pointsSize allows us to loop back to 0 when reaching end of points
        nextPoint = (currentPoint + 1) % pointsSize;

        for (int i = 0; i < pointsSize; i++)
        {
            if (i == currentPoint) continue; // Skip current point

            int o = orientation(controlPoints[currentPoint], controlPoints[i], controlPoints[nextPoint]);
            // If point i is counter clockwise OR collinear and further away than nextPoint
            if (o == 2 || (o == 0 && squaredDistance(controlPoints[currentPoint], controlPoints[i]) > squaredDistance(controlPoints[currentPoint], controlPoints[nextPoint])))
                nextPoint = i;

        }

        currentPoint = nextPoint;

    } while (currentPoint != leftmost);

    convexHull = resultHull;
}
std::pair<Bezier, Bezier> Bezier::subdivide(float t) const
{
    Bezier leftCurve, rightCurve;

    // Get the number of control points in the original curve
    int n = controlPoints.size();

    // Temporary array to store intermediate points during De Casteljau's algorithm
    std::vector<Vertex> temp = controlPoints;

    // These will store our new control points
    std::vector<Vertex> leftPoints(n);
    std::vector<Vertex> rightPoints(n);

    // The first control point of the left curve is the first control point of the original
    leftPoints[0] = controlPoints[0];

    // The last control point of the right curve is the last control point of the original
    rightPoints[n - 1] = controlPoints[n - 1];

    // Apply De Casteljau's algorithm
    for (int r = 1; r <= n - 1; r++)
    {
        // At each step, we calculate a new set of points
        for (int i = 0; i <= n - 1 - r; i++)
        {
            temp[i] = temp[i] * (1.0f - t) + temp[i + 1] * t;
        }

        // Store the leftmost point for the left curve
        leftPoints[r] = temp[0];

        // Store the rightmost point for the right curve
        rightPoints[n - 1 - r] = temp[n - 1 - r];
    }

    // Set the control points for our new curves
    for (int i = 0; i < n; i++)
    {
        leftCurve.addControlPoint(leftPoints[i]);
        rightCurve.addControlPoint(rightPoints[n - 1 - i]);  // We need to reverse this array
    }

    return { leftCurve, rightCurve };
}

float Bezier::calculateFlatness() const
{
    // Maximum distance from any control point to the line connecting endpoints
    float maxDistance = 0.0f;

    const Vertex& start = controlPoints.front();
    const Vertex& end = controlPoints.back();

    // Calculate the line from start to end
    float lineLength = std::sqrt(squaredDistance(start, end));

    // If the curve is a point or nearly a point, return zero flatness
    if (lineLength < 1e-6)
    {
        return 0.0f;
    }

    // For each control point (except start and end)
    for (size_t i = 1; i < controlPoints.size() - 1; i++)
    {
        const Vertex& point = controlPoints[i];

        // Calculate distance to line
        float distance = 0.0f;

        if (lineLength > 0)
        {
            // Vector from start to end
            float dx = end.x - start.x;
            float dy = end.y - start.y;

            // Calculate perpendicular distance
            distance = std::abs((point.y - start.y) * dx - (point.x - start.x) * dy) / lineLength;
        }

        maxDistance = std::max(maxDistance, distance);
    }

    return maxDistance;
}
