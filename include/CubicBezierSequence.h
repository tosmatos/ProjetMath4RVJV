#pragma once

#include "Bezier.h"
#include "Vertex.h"

#include <vector>

class CubicBezierSequence {
private:
    std::vector<Bezier> curves;
    int continuityType = 0; // 0 for C0, 1 for C1, 2 for C2

public:
    CubicBezierSequence(int continuityType = 0) : continuityType(continuityType) {};

    // Add a new curve to the sequence
    void addCurve(const Bezier& curve);

    std::vector<Bezier> getCurves() { return curves; };

    // Enforce continuity constraints on all curves
    void enforceConstraints();

    int getContinuityType() const { return continuityType; };
    // Set the continuity type (0=C0, 1=C1, 2=C2)
    void setContinuityType(int type);

    // Move a control point, maintaining continuity
    void moveControlPoint(int curveIndex, int pointIndex, const Vertex& newPosition);

    // Returns false if the point can be moved, considering current continuity type
    bool isConstrainedPoint(int curveIndex, int pointIndex);

    // Draw all curves
    void draw(Shader& shader) const;
};