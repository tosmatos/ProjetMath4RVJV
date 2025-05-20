#pragma once

#include <vector>

#include "PolyTypes.h"
#include "Polygon.h"
#include "Bezier.h"
#include "Matrix.h"
#include "IntersectionMarkers.h"

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
    Polygon polygon;
    Polygon window;
    std::vector<Polygon> finishedPolygons;
    std::vector<FilledPolygon> filledPolygons;

    void finishPolygon();
    
    Bezier bezier;
    std::vector<Bezier> finishedBeziers;

    void finishBezier();

    Vertex calculateCenter(const std::vector<Vertex>& vertices);

    std::vector<Vertex> transformOriginalVertices;
    bool isCurrentlyTransformingShape = false;

    // SAT implementation to test intersection on two convex shapes, our bézier hulls
    bool testHullIntersection(const std::vector<Vertex> shapeA, const std::vector<Vertex> shapeB);
    // Actual recursive subdivision implementation for finding bézier intersections, if hulls intersect
    std::vector<Vertex> findBezierIntersections(const Bezier& curve1, const Bezier& curve2,
        float floatnessThreshold, int maxDepth);
    IntersectionMarkers intersections;
    std::vector<std::string> foundIntersectionsText;

public:
    bool bezierMode;

    Polygon tempPolygon;
    Bezier tempBezier;

    // Function declarations
    void startPolygon(PolyType type);
    void appendVertex(double xPos, double yPos);
    void finish();
    void cancel();

    void deleteVertex(int shapeIndex, int vertexIndex, bool isPolygon);

    // Add a filled polygon to our storage
    void addFilledPolygon(const Polygon& poly,
        const std::vector<Vertex>& fillPoints,
        float r, float g, float b, float a);

    // Clear all filled polygons
    void clearFilledPolygons();

    const std::vector<Polygon>& getFinishedPolygons() const;

    void setFinishedPolygons(std::vector<Polygon> newFinishedPolygons);

    const std::vector<FilledPolygon>& getFilledPolygons() const;

    void addFinishedPolygon(const Polygon& polygon);

    void removeFinishedPolygon(int index);

    // For clipping purposes, removes already clipped polygons to make new ones
    void removeAllPolygonsOfType(PolyType type);

    // Access polygon by index (non-const version for modification)
    Polygon& getPolygonAt(size_t index);

    // Check if index is valid
    bool isValidPolygonIndex(int index) const;

    // Check if is currently building polygon, for mouse input
    bool isBuilding() const;

    // Bézier mode functions
    void toggleBezierMode() { bezierMode = !bezierMode; };
    void startBezierCurve();
    const std::vector<Bezier>& getFinishedBeziers() const { return finishedBeziers; };
    void removeFinishedBezier(size_t index);


    void swapBezierAlgorithm(size_t index);
    void incrementBezierStepSize(size_t index);
    void decrementBezierStepSize(size_t index);
    void toggleHullDisplay(size_t index);

    void duplicateControlPoint(int shapeIndex, int vertexIndex);

    // Translation is an affine transformation
    void translate(int shapeIndex, bool isPolygon, float deltaX, float deltaY);
    void translateVertex(int shapeIndex, int vertexIndex, bool isPolygon, float deltaX, float deltaY);

    // For linear transformations
    void startTransformingShape(int shapeIndex, bool isPolygon);
    void stopTransformingShape();

    void applyScaleFromOriginal(int shapeIndex, bool isPolygon, float totalScaleFactorX, float totalScaleFactorY);
    void applyRotationFromOriginal(int shapeIndex, bool isPolygon, float totalRotationAngle);
    void applyShearFromOriginal(int shapeIndex, bool isPolygon, float totalShearX, float totalShearY);

    void tryFindingIntersections();
    const std::vector<std::string> getFoundIntersectionsText() const { return foundIntersectionsText; };
    const IntersectionMarkers getIntersectionMarkers() const { return intersections; };
};