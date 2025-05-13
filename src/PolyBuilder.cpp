#include "PolyBuilder.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <iostream>
#include <string>

void PolyBuilder::startPolygon(PolyType type)
{
    polyType = type;
    buildingPoly = true;
    tempPolygon = Polygon();
}

void PolyBuilder::startBezierCurve()
{
    tempBezier = Bezier();
    buildingPoly = true;
    toggleBezierMode();
}

void PolyBuilder::removeFinishedBezier(size_t index)
{
    if (index < finishedBeziers.size())
        finishedBeziers.erase(finishedBeziers.begin() + index);
}

void PolyBuilder::swapBezierAlgorithm(size_t index)
{
    if (index < finishedBeziers.size())
        finishedBeziers[index].swapAlgorithm();
}

void PolyBuilder::incrementBezierStepSize(size_t index)
{
    if (index < finishedBeziers.size())
        finishedBeziers[index].incrementStepSize();
}

void PolyBuilder::decrementBezierStepSize(size_t index)
{
    if (index < finishedBeziers.size())
        finishedBeziers[index].decrementStepSize();
}

void PolyBuilder::translate(int shapeIndex, bool isPolygon, float deltaX, float deltaY)
{
    Matrix3x3 translationMatrix = createTranslationMatrix(deltaX, deltaY);

    std::vector<Vertex> vertices;
    if (isPolygon)
    {
        auto poly = finishedPolygons[shapeIndex];
        vertices = poly.getVertices();
    }
    else
    {
        auto bezier = finishedBeziers[shapeIndex];
        vertices = bezier.getControlPoints();
    }

    for (Vertex& vertex : vertices)
    {
        Vertex transformedPoint = multiplyMatrixVertex(translationMatrix, vertex);
        vertex.x = transformedPoint.x;
        vertex.y = transformedPoint.y;
    }

    if (isPolygon)
    {
        Polygon& poly = finishedPolygons[shapeIndex];
        poly.setVertices(vertices);
        poly.updateBuffers();
    }
    else
    {
        Bezier& bezier = finishedBeziers[shapeIndex];
        bezier.setControlPoints(vertices);
        bezier.generateCurve(); // Updates buffers internally
    }
}

void PolyBuilder::rotate(int shapeIndex, bool isPolygon, float deltaX, float deltaY)
{
    // TODO : implement this !
}

// In PolyBuilder.cpp

void PolyBuilder::startScalingShape(int shapeIndex, bool isPolygon)
{
    if (isPolygon)
    {
        if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size())
        {
            transformOriginalVertices = finishedPolygons[shapeIndex].getVertices();
            isCurrentlyScalingShape = true;
        }
    }
    else
    {
        if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size())
        {
            transformOriginalVertices = finishedBeziers[shapeIndex].getControlPoints();
            isCurrentlyScalingShape = true;
        }
    }
}

void PolyBuilder::stopScalingShape()
{
    isCurrentlyScalingShape = false;
    transformOriginalVertices.clear();
}

// This will replace your current `scale` method for this drag interaction
void PolyBuilder::applyScaleFromOriginal(int shapeIndex, bool isPolygon, float totalScaleFactorX, float totalScaleFactorY)
{
    if (!isCurrentlyScalingShape || transformOriginalVertices.empty())
    {
        return; // Not in a scaling operation or no original vertices
    }

    //std::cout << "In applyScaleFromOriginal. shapeIndex : " << shapeIndex << std::endl;

    std::vector<Vertex> newVertices = transformOriginalVertices; // Work on a copy of the original

    Vertex center = calculateCenter(transformOriginalVertices); // Center of the *original* shape

    Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
    Matrix3x3 scalingMatrix = createScalingMatrix(totalScaleFactorX, totalScaleFactorY);
    Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);

    Matrix3x3 finalMatrix = translateBack * scalingMatrix * translateToOrigin;

    for (Vertex& vertex : newVertices)
    {
        // Apply the total transformation to each original vertex
        Vertex transformedPoint = multiplyMatrixVertex(finalMatrix, vertex);
        vertex.x = transformedPoint.x;
        vertex.y = transformedPoint.y;
    }

    // Update the live shape with the newly calculated vertices
    if (isPolygon)
    {
        if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size())
        {
            Polygon& poly = finishedPolygons[shapeIndex];
            poly.setVertices(newVertices);
            poly.updateBuffers();
        }
    }
    else
    {
        if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size())
        {
            Bezier& bezier = finishedBeziers[shapeIndex];
            bezier.setControlPoints(newVertices);
            bezier.generateCurve(); // This should update its buffers
        }
    }
}

void PolyBuilder::scale(int shapeIndex, bool isPolygon, float scaleFactorX, float scaleFactorY)
{
    std::vector<Vertex> vertices;
    if (isPolygon)
    {
        auto poly = finishedPolygons[shapeIndex];
        vertices = poly.getVertices();
    }
    else
    {
        auto bezier = finishedBeziers[shapeIndex];
        vertices = bezier.getControlPoints();
    }

    Vertex center = calculateCenter(vertices);

    Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
    Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
    Matrix3x3 scalingMatrix = createScalingMatrix(scaleFactorX, scaleFactorY);

    Matrix3x3 finalMatrix = translateBack * scalingMatrix * translateToOrigin;

    for (Vertex& vertex : vertices)
    {
        Vertex transformedPoint = multiplyMatrixVertex(finalMatrix, vertex);
        vertex.x = transformedPoint.x;
        vertex.y = transformedPoint.y;
    }

    if (isPolygon)
    {
        Polygon& poly = finishedPolygons[shapeIndex];
        poly.setVertices(vertices);
        poly.updateBuffers();
    }
    else
    {
        Bezier& bezier = finishedBeziers[shapeIndex];
        bezier.setControlPoints(vertices);
        bezier.generateCurve(); // Updates buffers internally
    }
}

void PolyBuilder::shear(int shapeIndex, bool isPolygon, float deltaX, float deltaY)
{
    Matrix3x3 shearingMatrix = createShearingMatrix(deltaX, deltaY);

    std::vector<Vertex> vertices;
    if (isPolygon)
    {
        auto poly = finishedPolygons[shapeIndex];
        vertices = poly.getVertices();
    }
    else
    {
        auto bezier = finishedBeziers[shapeIndex];
        vertices = bezier.getControlPoints();
    }

    for (Vertex& vertex : vertices)
    {
        Vertex transformedPoint = multiplyMatrixVertex(shearingMatrix, vertex);
        vertex.x = transformedPoint.x;
        vertex.y = transformedPoint.y;
    }

    if (isPolygon)
    {
        Polygon& poly = finishedPolygons[shapeIndex];
        poly.setVertices(vertices);
        poly.updateBuffers();
    }
    else
    {
        Bezier& bezier = finishedBeziers[shapeIndex];
        bezier.setControlPoints(vertices);
        bezier.generateCurve(); // Updates buffers internally
    }
}

void PolyBuilder::appendVertex(double xPos, double yPos)
{
    if (!buildingPoly)
        return;

    // Get window size
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

    // Convert to normalized device coordinates (-1 to 1)
    float normalizedX = (2.0f * xPos / width) - 1.0f;
    float normalizedY = 1.0f - (2.0f * yPos / height); // Flip Y coordinate

    std::cout << "Added a vertex at X : " << std::to_string(normalizedX) << ", Y : " << std::to_string(normalizedY) <<
        std::endl;

    if (bezierMode)
    {
        tempBezier.addControlPoint(normalizedX, normalizedY);
        tempBezier.updateBuffers();
        if (tempBezier.getControlPoints().size() > 2)
            tempBezier.generateCurve();
    }
    else
    {
        tempPolygon.addVertex(normalizedX, normalizedY);
        tempPolygon.updateBuffers();
    }
        
}

void PolyBuilder::finish()
{
    if (bezierMode)
        finishBezier();
    else
        finishPolygon();
}

void PolyBuilder::finishPolygon()
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

void PolyBuilder::finishBezier()
{
    std::cout << "Finishing Bézier ..." << std::endl;

    if (!buildingPoly)
        return;

    bezier = tempBezier;
    bezier.generateCurve(); // Generate buffers is called from generate curve
    finishedBeziers.push_back(bezier);
    buildingPoly = false;
    toggleBezierMode();
    tempBezier = Bezier();
}

// For transformations that should take place on the center, like scaling or shearing
Vertex PolyBuilder::calculateCenter(const std::vector<Vertex>& vertices)
{
    if (vertices.empty()) return { 0.0f, 0.0f };
    float sumX = 0.0f;
    float sumY = 0.0f;
    for (const auto& vert : vertices)
    {
        sumX += vert.x;
        sumY += vert.y;
    }
    return { sumX / vertices.size(), sumY / vertices.size() };
}

void PolyBuilder::cancel()
{
    buildingPoly = false;
    tempPolygon = Polygon();
    tempBezier = Bezier();
}


void PolyBuilder::movePolygon(int polyIndex, float deltaX, float deltaY)
{
    // Check for valid index
    if (polyIndex < 0 || polyIndex >= finishedPolygons.size())
        return;

    // Get a reference to the polygon to modify
    Polygon& poly = finishedPolygons[polyIndex];

    // Get a modifiable copy of the vertices
    std::vector<Vertex> modifiedVertices = poly.getVertices();

    // Apply translation to each vertex
    for (auto& vertex : modifiedVertices)
    {
        vertex.x += deltaX;
        vertex.y += deltaY;
    }

    // Update the polygon with the new vertices
    poly.setVertices(modifiedVertices);

    // Re-initialize the OpenGL buffers
    poly.updateBuffers();
}

void PolyBuilder::updateVertexPosition(int shapeIndex, int vertexIndex, bool isPolygon, float deltaX, float deltaY)
{
    //std::cout << "Updating vertex position" << std::endl;

    std::vector<Vertex> vertices;
    if (isPolygon)
    {
        auto poly = finishedPolygons[shapeIndex];
        vertices = poly.getVertices();
    }
    else
    {
        auto bezier = finishedBeziers[shapeIndex];
        vertices = bezier.getControlPoints();
    }

    Vertex vertexToUpdate = vertices[vertexIndex];
    vertexToUpdate.x += deltaX;
    vertexToUpdate.y += deltaY;
    vertices[vertexIndex] = vertexToUpdate;

    if (isPolygon)
    {
        Polygon& poly = finishedPolygons[shapeIndex];
        poly.setVertices(vertices);
        poly.updateBuffers();
    }
    else
    {
        Bezier& bezier = finishedBeziers[shapeIndex];
        bezier.setControlPoints(vertices);
        bezier.generateCurve(); // calls update buffers internally
    }
}

void PolyBuilder::deleteVertex(int shapeIndex, int vertexIndex, bool isPolygon)
{
    std::vector<Vertex> vertices;

    if (isPolygon)
    {
        if (shapeIndex < 0 || shapeIndex >= finishedPolygons.size())
        {
            std::cerr << "Error: Invalid polygon index " << shapeIndex << std::endl;
            return;
        }

        Polygon& poly = finishedPolygons[shapeIndex];
        vertices = poly.getVertices();

        // Check vertex index validity
        if (vertexIndex < 0 || vertexIndex >= vertices.size())
        {
            std::cerr << "Error: Invalid vertex index " << vertexIndex << " for polygon " << shapeIndex << std::endl;
            return;
        }

        vertices.erase(vertices.begin() + vertexIndex);

        if (vertices.size() < 3)
        {
            std::cout << "Polygon " << shapeIndex << " has less than 3 vertices after deletion. Removing polygon." << std::endl;
            finishedPolygons.erase(finishedPolygons.begin() + shapeIndex);
            return; // Polygon removed
        }

        poly.setVertices(vertices);
        poly.updateBuffers();
    }
    else
    {
        // Check index validity for the Bezier list
        if (shapeIndex < 0 || shapeIndex >= finishedBeziers.size())
        {
            std::cerr << "Error: Invalid Bezier index " << shapeIndex << std::endl;
            return;
        }
        Bezier& bezier = finishedBeziers[shapeIndex];
        vertices = bezier.getControlPoints();

        // Check vertex index validity
        if (vertexIndex < 0 || vertexIndex >= vertices.size())
        {
            std::cerr << "Error: Invalid control point index " << vertexIndex << " for Bezier " << shapeIndex << std::endl;
            return;
        }

        vertices.erase(vertices.begin() + vertexIndex);

        if (vertices.size() < 2)
        {
            std::cout << "Bezier " << shapeIndex << " has less than 2 control points after deletion. Removing Bezier." << std::endl;
            finishedBeziers.erase(finishedBeziers.begin() + shapeIndex);
            return; // Bezier removed
        }

        bezier.setControlPoints(vertices);
        bezier.generateCurve(); // Calls update buffers internally
    }
}

// Add a filled polygon to our storage
void PolyBuilder::addFilledPolygon(const Polygon& poly,
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
void PolyBuilder::clearFilledPolygons()
{
    for (auto& filled : filledPolygons)
    {
        // Delete OpenGL resources
        if (filled.vao != 0)
        {
            glDeleteVertexArrays(1, &filled.vao);
            glDeleteBuffers(1, &filled.vbo);
        }
    }
    filledPolygons.clear();
}

const std::vector<Polygon>& PolyBuilder::getFinishedPolygons() const
{
    return finishedPolygons;
}

void PolyBuilder::setFinishedPolygons(std::vector<Polygon> newFinishedPolygons)
{
    finishedPolygons = newFinishedPolygons;
}

const std::vector<FilledPolygon>& PolyBuilder::getFilledPolygons() const
{
    return filledPolygons;
}

void PolyBuilder::addFinishedPolygon(const Polygon& polygon)
{
    finishedPolygons.push_back(polygon);
}

void PolyBuilder::removeFinishedPolygon(int index)
{
    if (index >= 0 && index < finishedPolygons.size())
        finishedPolygons.erase(finishedPolygons.begin() + index);
}

void PolyBuilder::removeAllPolygonsOfType(PolyType type)
{
    auto it = finishedPolygons.begin();
    while (it != finishedPolygons.end()) {
        if (it->type == type)
            it = finishedPolygons.erase(it);
        else
            ++it;
    }
}

Polygon& PolyBuilder::getPolygonAt(size_t index)
{
    if (index >= finishedPolygons.size())
        throw std::out_of_range("Polygon index out of range");

    return finishedPolygons[index];
}

bool PolyBuilder::isValidPolygonIndex(int index) const
{
    return index >= 0 && index < static_cast<int>(finishedPolygons.size());
}

bool PolyBuilder::isBuilding() const
{
    return buildingPoly;
}
