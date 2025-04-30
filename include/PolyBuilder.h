#pragma once

#include <vector>

#include "PolyTypes.h"
#include "Polygon.h"
#include "Bezier.h"

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
    PolyType polyType;
    bool buildingPoly;
    Polygon tempPolygon;
    Polygon polygon;
    Polygon window;
    std::vector<Polygon> finishedPolygons;
    std::vector<FilledPolygon> filledPolygons;

    void FinishPolygon();
    
    bool bezierMode;
    Bezier tempBezier;
    Bezier bezier;
    std::vector<Bezier> finishedBeziers;

    void FinishBezier();

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

    const std::vector<Polygon>& GetFinishedPolygons() const;

    void SetFinishedPolygons(std::vector<Polygon> newFinishedPolygons);

    const std::vector<FilledPolygon>& GetFilledPolygons() const;

    void AddFinishedPolygon(const Polygon& polygon);

    void RemoveFinishedPolygon(int index);

    // For clipping purposes, removes already clipped polygons to make new ones
    void RemoveAllPolygonsOfType(PolyType type);

    // Access polygon by index (non-const version for modification)
    Polygon& GetPolygonAt(size_t index);

    // Check if index is valid
    bool IsValidPolygonIndex(int index) const;

    // Check if is currently building polygon, for mouse input
    bool IsBuilding() const;

    // Bézier mode functions
    void ToggleBezierMode() { bezierMode = !bezierMode; };
    void StartBezierCurve();
    const std::vector<Bezier>& GetFinishedBeziers() const { return finishedBeziers; };
    void RemoveFinishedBezier(size_t index);

    void SwapBezierAlgorithm(size_t index);
    void IncrementBezierStepSize(size_t index);
    void DecrementBezierStepSize(size_t index);
};