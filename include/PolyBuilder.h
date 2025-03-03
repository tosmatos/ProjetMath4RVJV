#pragma once

#include "PolyTypes.h"
#include "Polygon.h"
#include <vector>

namespace PolyBuilder {
    // Declare variables as external
    extern Type polyType;
    extern bool buildingPoly;
    extern Polygon tempPolygon;
    extern Polygon polygon;
    extern Polygon window;
    extern std::vector<Polygon> finishedPolygons;

    // For storing filled polygons
    struct FilledPolygon {
        Polygon polygon;       // The original polygon
        std::vector<Vertex> fillPoints; // The points to fill
        float colorR, colorG, colorB, colorA; // Fill color
        unsigned int vao, vbo; // OpenGL handles for the fill points

        FilledPolygon() : vao(0), vbo(0),
            colorR(0.0f), colorG(0.0f),
            colorB(1.0f), colorA(1.0f) {}
    };

    extern std::vector<FilledPolygon> filledPolygons;

    // Function declarations
    void StartPolygon(Type type);
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
}