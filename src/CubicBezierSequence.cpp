#include "CubicBezierSequence.h"
#include "MathUtils.h"

#include <cmath>
#include <iostream>

using namespace MathUtils;

void CubicBezierSequence::addCurve(const Bezier& curve)
{
    curves.push_back(curve);
}

void CubicBezierSequence::incrementStepSize()
{
    for (auto& curve : curves)
    {
        curve.incrementStepSize();
    }
    stepSize = curves[0].getStepSize(); // Replicate change on this
    calculateGenerationTime();
}

void CubicBezierSequence::decrementStepSize()
{
    for (auto& curve : curves)
    {
        curve.decrementStepSize();
    }
    stepSize = curves[0].getStepSize();
    calculateGenerationTime();
}

void CubicBezierSequence::swapAlgorithm()
{
    for (auto& curve : curves)
    {
        curve.swapAlgorithm();
    }
    algorithm = curves[0].getAlgorithm();
    calculateGenerationTime();
}

void CubicBezierSequence::calculateGenerationTime()
{
    generationTime = 0.0f;
    for (auto curve : curves)
    {
        std::cout << "Generation time for curve : " << curve.getGenerationTime() << std::endl;
        generationTime += curve.getGenerationTime();
    }
}

void CubicBezierSequence::enforceConstraints() {
    if (curves.size() < 2) return;

    // Constants to control curve shape
    const float percentage = 0.4f;      // Use 40% of previous segment length
    const float maxDistance = 0.2f;     // Maximum control point distance (in normalized coords)

    for (size_t i = 1; i < curves.size(); i++) {
        Bezier& prevCurve = curves[i - 1];
        Bezier& nextCurve = curves[i];

        std::vector<Vertex> prevCurveControlPoints = prevCurve.getControlPoints();
        std::vector<Vertex> nextCurveControlPoints = nextCurve.getControlPoints();

        if (prevCurveControlPoints.size() != 4 || nextCurveControlPoints.size() != 4)
            continue;

        const Vertex& P1 = prevCurveControlPoints[1];
        const Vertex& P2 = prevCurveControlPoints[2];
        const Vertex& P3 = prevCurveControlPoints[3]; // Same as Q0

        // C0 continuity - curves meet at a point
        nextCurveControlPoints[0] = P3;

        if (continuityType >= 1) {
            // Calculate tangent vector
            Vertex tangent = P3 - P2;
            float tangentLength = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);

            if (tangentLength > 0.001f) {
                // Normalize the tangent
                Vertex tangentDir = tangent * (1.0f / tangentLength);

                // Calculate desired length with absolute cap
                float desiredLength = std::min(tangentLength * percentage, maxDistance);

                // Place Q1 along the tangent with controlled length
                nextCurveControlPoints[1] = P3 + tangentDir * desiredLength;

                if (continuityType >= 2) {
                    // For C2, we need to respect curvature but control magnitude

                    // Get distance from P1 to P2
                    float distP1P2 = std::sqrt(
                        (P2.x - P1.x) * (P2.x - P1.x) +
                        (P2.y - P1.y) * (P2.y - P1.y)
                    );

                    // Limit the influence of P1
                    float cappedP1P2 = std::min(distP1P2, maxDistance);

                    // Apply C2 constraint with controlled magnitude
                    Vertex Q1 = nextCurveControlPoints[1];

                    // Use a simplified C2 formula that prevents compounding
                    float Q1Q2Length = std::min(desiredLength, maxDistance * 0.8f);

                    // Place Q2 to maintain C2 but with bounded distance
                    nextCurveControlPoints[2] = Q1 + tangentDir * Q1Q2Length;
                }
            }
        }

        nextCurve.setControlPoints(nextCurveControlPoints);
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

void CubicBezierSequence::drawPreview(Shader& shader) const
{
    for (auto curve : curves)
    {
        curve.drawControlPointsPreview(shader);
        curve.drawGeneratedCurvePreview(shader);
    }
}