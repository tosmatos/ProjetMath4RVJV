#include "Bezier.h"
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

Bezier::Bezier()
{

}

Bezier::Bezier(const Bezier& other) : controlPoints(other.controlPoints), buffersInitialized(false)
{ // Copy constructor
    controlPoints = other.controlPoints;
    
    generatedCurve = other.generatedCurve;
    convexHull = other.convexHull;
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

const void Bezier::drawConvexHull(Shader& shader) const
{
    shader.use();
    glBindVertexArray(hullVAO);

    shader.setColor("uColor", 0.0f, 1.0f, 0.5f, 0.25f);
    glDrawArrays(GL_LINE_LOOP, 0, convexHull.size());

    shader.setColor("uColor", 1.0f, 1.0f, 1.0f, 0.25f);
    glDrawArrays(GL_POINTS, 0, convexHull.size());
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
    std::vector<Vertex> resultHull;
    int pointsSize = controlPoints.size();
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
    updateBuffers();
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

bool Bezier::lineSegmentsIntersect(const Vertex& segmentA_start, const Vertex& segmentA_end,
    const Vertex& segmentB_start, const Vertex& segmentB_end, Vertex& intersectionPoint)
{
    // Calculate the direction vectors of our two line segments
    Vertex directionA(segmentA_end.x - segmentA_start.x, segmentA_end.y - segmentA_start.y);
    Vertex directionB(segmentB_end.x - segmentB_start.x, segmentB_end.y - segmentB_start.y);

    // We'll use the cross product to determine if the lines are parallel
    // TODO : Cross product exists in Clipper.cpp. Find a way to make it clean and portable
    float crossProduct = directionA.x * directionB.y - directionA.y * directionB.x;

    // If the cross product is nearly zero, the lines are parallel or collinear
    // We use a small epsilon value to account for floating point precision
    if (std::abs(crossProduct) < 1e-6)
        return false;  // Parallel lines don't intersect

    // Calculate the vector from start of segment A to start of segment B
    Vertex startDifference(segmentB_start.x - segmentA_start.x, segmentB_start.y - segmentA_start.y);

    // Calculate how far along segment A the intersection occurs (from 0 to 1)
    // We use the formula: t = (startDifference × directionB) / (directionA × directionB)
    // Where × is the 2D cross product
    float intersectionRatioA = (startDifference.x * directionB.y - startDifference.y * directionB.x) / crossProduct;

    // Calculate how far along segment B the intersection occurs (from 0 to 1)
    // We use the formula: u = (startDifference × directionA) / (directionA × directionB)
    float intersectionRatioB = (startDifference.x * directionA.y - startDifference.y * directionA.x) / crossProduct;

    // Check if the intersection point is within both segments
    // For the intersection to be on both segments, both ratios must be between 0 and 1
    if (intersectionRatioA >= 0 && intersectionRatioA <= 1 && intersectionRatioB >= 0 && intersectionRatioB <= 1)
    {
        // Calculate the intersection point using the ratio along segment A
        // Doesn't matter if we use intersectionRatioA or B for getting the point
        intersectionPoint.x = segmentA_start.x + intersectionRatioA * directionA.x;
        intersectionPoint.y = segmentA_start.y + intersectionRatioA * directionA.y;
        return true;  // We found an intersection!
    }

    return false;
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
