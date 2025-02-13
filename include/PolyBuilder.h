#pragma once

#include "PolyTypes.h"
#include "Polygon.h"

namespace PolyBuilder {
    // Declare variables as external
    extern Type polyType;
    extern bool buildingPoly;
    extern Polygon tempPolygon;
    extern Polygon polygon;
    extern Polygon window;
    extern std::vector<Polygon> finishedPolygons;

    // Function declarations
    void StartPolygon(Type type);
    void AppendVertex(double xPos, double yPos);
    void Finish();
    void Cancel();
}