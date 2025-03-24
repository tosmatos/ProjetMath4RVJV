#pragma once

#include "PolyTypes.h"
#include "Polygon.h"
#include <vector>

// For storing filled polygons
struct FilledPolygon
{
    Polygon polygon;       // The original polygon
    std::vector<Vertex> fillPoints; // The points to fill
    float colorR, colorG, colorB, colorA; // Fill color
    unsigned int vao, vbo; // OpenGL handles for the fill points

    FilledPolygon() : vao(0), vbo(0),
        colorR(0.0f), colorG(0.0f),
        colorB(1.0f), colorA(1.0f)
    {}
};

class PolyBuilder
{
private:
    // Declare variables as external
    PolyType polyType;
    bool buildingPoly;
    Polygon tempPolygon;
    Polygon polygon;
    Polygon window;
    std::vector<Polygon> finishedPolygons;
    std::vector<FilledPolygon> filledPolygons;

public:
    // Function declarations
    void StartPolygon(PolyType type);
    void AppendVertex(double xPos, double yPos);
    void Finish();
    void Cancel();

    // New function for polygon transformation
    void MovePolygon(int polyIndex, float deltaX, float deltaY);

    // Add a filled polygon to our storage
    void AddFilledPolygon(const Polygon& poly,
        const std::vector<Vertex>& fillPoints,
        float r, float g, float b, float a);

    // Clear all filled polygons
    void ClearFilledPolygons();

    const std::vector<Polygon>& GetFinishedPolygons() const
    {
        return finishedPolygons;
    }

    void SetFinishedPolygons(std::vector<Polygon> newFinishedPolygons)
    {
        finishedPolygons = newFinishedPolygons;
    }

    const std::vector<FilledPolygon>& GetFilledPolygons() const
    {
        return filledPolygons;
    }

    bool IsBuilding() const
    {
        return buildingPoly;
    }
};