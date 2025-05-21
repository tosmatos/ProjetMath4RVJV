#pragma once

#include <vector>

#include "CommonTypes.h"
#include "Polygon.h"
#include "Bezier.h"
#include "CubicBezierSequence.h"
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

// Struct for sequences transformation
// Allows for "easy" mapping of transformed vertices from all the curves of a sequence
struct SequenceTransformData {
    std::vector<std::vector<Vertex>> originalCurvePoints;
};

class PolyBuilder
{
private:
    PolyType polyType;
    bool buildingShape;    
    Polygon polygon;
    Polygon window;
    std::vector<Polygon> finishedPolygons;
    std::vector<FilledPolygon> filledPolygons;

    void finishPolygon();
    
    Bezier bezier;
    std::vector<Bezier> finishedBeziers;

    void finishBezier();
    
    int continuityType = 0;    
    std::vector<CubicBezierSequence> finishedSequences;

    Vertex calculateCenter(const std::vector<Vertex>& vertices);

    std::vector<Vertex> transformOriginalVertices;
    SequenceTransformData sequenceTransformData;
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
    bool cubicSequenceMode;

    Polygon tempPolygon;
    Bezier tempBezier;
    CubicBezierSequence currentSequence;

    // Function declarations
    void startPolygon(PolyType type);
    void appendVertex(double xPos, double yPos);
    void finish();
    void cancel();

    void deleteVertex(int shapeIndex, int vertexIndex, ShapeType shapeType);

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

    void startCubicSequence();
    void appendToCubicSequence(float x, float y);
    void finishCubicSequence();

    void toggleCubicSequenceMode() { cubicSequenceMode = !cubicSequenceMode; };
    int getContinuityType() const { return continuityType; };
    void setContinuityType(int type) { if (type >= 0 && type <= 2) continuityType = type; };
    std::vector<CubicBezierSequence>& getFinishedBezierSequences() { return finishedSequences; };

    void swapSequenceAlgorithm(size_t index);
    void incrementSequenceStepSize(size_t index);
    void decrementSequenceStepSize(size_t index);
    void removeFinishedSequence(size_t index);
    void curveToPolygon(size_t index);

    void duplicateControlPoint(int shapeIndex, int vertexIndex);

    // Translation is an affine transformation
    void translate(int shapeIndex, ShapeType shapeType, float deltaX, float deltaY);
    void translateVertex(int shapeIndex, int vertexIndex, ShapeType shapeType, float deltaX, float deltaY);

    // For linear transformations
    void startTransformingShape(int shapeIndex, ShapeType shapeType);
    void stopTransformingShape();

    void applyScaleFromOriginal(int shapeIndex, ShapeType shapeType, float totalScaleFactorX, float totalScaleFactorY);
    void applyRotationFromOriginal(int shapeIndex, ShapeType shapeType, float totalRotationAngle);
    void applyShearFromOriginal(int shapeIndex, ShapeType shapeType, float totalShearX, float totalShearY);

    void tryFindingIntersections();
    const std::vector<std::string> getFoundIntersectionsText() const { return foundIntersectionsText; };
    const IntersectionMarkers getIntersectionMarkers() const { return intersections; };
    const void drawIntersectionMarkers(Shader& shader) const { intersections.draw(shader); };

    Polygon createPolygonFromBezierSequence(const CubicBezierSequence& bezierSequence);
};