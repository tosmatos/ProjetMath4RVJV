#include "CubicBezierSequence.h"

void CubicBezierSequence::addCurve(const Bezier& curve)
{
    curves.push_back(curve);
}

void CubicBezierSequence::enforceConstraints()
{
    if (curves.size() < 2) return;

    for (size_t i = 1; i < curves.size(); i++) {
        Bezier& prevCurve = curves[i - 1];
        Bezier& nextCurve = curves[i];
        std::vector<Vertex> prevCurveControlPoints = prevCurve.getControlPoints();
        std::vector<Vertex> nextCurveControlPoints = nextCurve.getControlPoints();

        // Make sure both curves have 4 control points (cubic)
        if (prevCurveControlPoints.size() != 4 || nextCurveControlPoints.size() != 4)
            continue;

        // C0 continuity (always enforce this)
        nextCurveControlPoints[0] = prevCurveControlPoints[3];

        if (continuityType >= 1) {
            // C1 continuity
            // Calculate Q1 based on P3 and P2
            Vertex P2 = prevCurveControlPoints[2];
            Vertex P3 = prevCurveControlPoints[3]; // Same as Q0

            // Q1 = P3 + (P3 - P2) = 2*P3 - P2
            nextCurveControlPoints[1] = P3 * 2.0f - P2;

            if (continuityType >= 2) {
                // C2 continuity
                Vertex P1 = prevCurveControlPoints[1];
                // Q2 = 2*Q1 - Q0 - P2 + P1
                nextCurveControlPoints[2] = nextCurveControlPoints[1] * 2.0f -
                    nextCurveControlPoints[0] - P2 + P1;
            }
        }

        prevCurve.setControlPoints(prevCurveControlPoints);
        nextCurve.setControlPoints(nextCurveControlPoints);
        // Regenerate the curve after modifying control points
        nextCurve.generateCurve();
    }
}

void CubicBezierSequence::setContinuityType(int type)
{
    if (type >= 0 && type <= 2)
        continuityType = type;
}

void CubicBezierSequence::moveControlPoint(int curveIndex, int pointIndex, const Vertex& newPosition)
{
    // TODO : Figure out it this should be done in PolyBuilder instead
}

bool CubicBezierSequence::isConstrainedPoint(int curveIndex, int pointIndex)
{
    if (curveIndex == 0) {
        // First curve has no backward constraints
        return false;
    }

    // For all other curves:
    if (pointIndex == 0) return true; // Q0 is always constrained (C0)
    if (pointIndex == 1 && continuityType >= 1) return true; // Q1 constrained in C1, C2
    if (pointIndex == 2 && continuityType >= 2) return true; // Q2 constrained in C2

    return false;
}

void CubicBezierSequence::draw(Shader& shader) const
{
    for (auto curve : curves)
    {
        curve.drawControlPoints(shader);
        curve.drawGeneratedCurve(shader);
    }
}
