#include "CubicBezierSequence.h"
#include "MathUtils.h"

#include <cmath>
#include <iostream>

using namespace MathUtils;

CubicBezierSequence::~CubicBezierSequence()
{
    // Vector handles everything by itself
}

CubicBezierSequence& CubicBezierSequence::operator=(const CubicBezierSequence& other) {
    if (this != &other) {
        curves.clear();  // Clear existing curves

        // Deep copy all curves
        for (const auto& curve : other.curves) {
            curves.push_back(curve);  // This will use Bezier's copy constructor
        }

        // Copy other member variables
        stepSize = other.stepSize;
        algorithm = other.algorithm;
        generationTime = other.generationTime;
        continuityType = other.continuityType;
        isClosed = other.isClosed;
    }
    return *this;
}

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
    for (const auto& curve : curves)
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

        if (i >= curves.size() ||
            prevCurveControlPoints.size() != 4 ||
            nextCurveControlPoints.size() != 4) {
            continue; // Skip this iteration if data isn't as expected
        }

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
                    // Calculate second derivative of first curve at P3
                    Vertex secondDerivP = P3 - P2 * 2 + P1;

                    // Scale it by k1^2 (k1 is tangent ratio)
                    float k1 = desiredLength / tangentLength;
                    float k1Squared = k1 * k1;

                    // Apply C2 formula: Q2 = 2*Q1 - Q0 + k1^2*(P3-2*P2+P1)
                    nextCurveControlPoints[2] = nextCurveControlPoints[1] * 2 - nextCurveControlPoints[0]
                        + secondDerivP * k1Squared;

                    // Apply max distance constraint if needed
                    Vertex Q1Q2 = nextCurveControlPoints[2] - nextCurveControlPoints[1];
                    float Q1Q2Length = std::sqrt(Q1Q2.x * Q1Q2.x + Q1Q2.y * Q1Q2.y);

                    if (Q1Q2Length > maxDistance) {
                        Q1Q2 = Q1Q2 * (maxDistance / Q1Q2Length);
                        nextCurveControlPoints[2] = nextCurveControlPoints[1] + Q1Q2;
                    }
                }
            }
        }

        nextCurve.setControlPoints(nextCurveControlPoints);
        nextCurve.generateCurve();
        nextCurve.updateBuffers();
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

bool CubicBezierSequence::isConstrainedPoint(int curveIndex, int pointIndex) const
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

void CubicBezierSequence::makeClosed() {
    if (curves.size() < 1) return;

    // Get first and last curves
    Bezier& firstCurve = curves[0];
    Bezier& lastCurve = curves[curves.size() - 1];

    // Get control points
    std::vector<Vertex> lastControlPoints = lastCurve.getControlPoints();
    std::vector<Vertex> firstControlPoints = firstCurve.getControlPoints();

    // Enforce C0 continuity: Make last point match first point
    lastControlPoints[3] = firstControlPoints[0];

    // Constants to control curve shape - similar to your enforceConstraints method
    const float percentage = 0.4f;      // Use 40% of previous segment length
    const float maxDistance = 0.2f;     // Maximum control point distance

    if (continuityType >= 1) {
        // C1 continuity: Make incoming/outgoing tangents align
        // Calculate tangent at the beginning of the first curve
        Vertex tangent = firstControlPoints[1] - firstControlPoints[0];
        float tangentLength = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);

        if (tangentLength > 0.001f) {
            // Normalize the tangent
            Vertex tangentDir = tangent * (1.0f / tangentLength);

            // Calculate desired length with absolute cap
            float desiredLength = std::min(tangentLength * percentage, maxDistance);

            // Place the second-to-last control point to create the same tangent
            // but in the opposite direction
            lastControlPoints[2] = lastControlPoints[3] - tangentDir * desiredLength;

            if (continuityType >= 2) {
                // C2 continuity: Match curvature at the junction
                // Get second derivatives
                Vertex secondDerivFirst = firstControlPoints[2] - firstControlPoints[1] * 2.0f + firstControlPoints[0];

                // Scale the second derivative by the squared ratio of the tangent lengths
                float k1 = desiredLength / tangentLength;
                float k1Squared = k1 * k1;

                // Apply C2 formula (derived from matching second derivatives)
                // Q1 = junction point (P3 or Q0)
                // Q2 = lastControlPoints[2] (already set for C1)
                // We need to set Q3 to maintain C2 continuity
                lastControlPoints[1] = lastControlPoints[2] * 2.0f - lastControlPoints[3]
                    + secondDerivFirst * k1Squared;

                // Apply max distance constraint if needed
                Vertex Q2Q1 = lastControlPoints[1] - lastControlPoints[2];
                float Q2Q1Length = std::sqrt(Q2Q1.x * Q2Q1.x + Q2Q1.y * Q2Q1.y);

                if (Q2Q1Length > maxDistance) {
                    Q2Q1 = Q2Q1 * (maxDistance / Q2Q1Length);
                    lastControlPoints[1] = lastControlPoints[2] + Q2Q1;
                }
            }
        }
    }

    // Update the last curve with modified control points
    lastCurve.setControlPoints(lastControlPoints);
    lastCurve.generateCurve();
    lastCurve.updateBuffers();
    isClosed = true;
}

bool CubicBezierSequence::shouldBeClosed() const {
    if (curves.size() < 1) return false;

    const Vertex& firstPoint = curves.front().getControlPoints().front();
    const Vertex& lastPoint = curves.back().getControlPoints().back();

    // Calculate squared distance
    float dx = firstPoint.x - lastPoint.x;
    float dy = firstPoint.y - lastPoint.y;
    float squaredDist = squaredDistance(dx, dy);

    // Threshold for considering points identical
    const float threshold = 0.001f;
    return squaredDist < threshold;
}

void CubicBezierSequence::draw(Shader& shader) const
{
    for (const auto& curve : curves)
    {
        curve.drawControlPoints(shader);
        curve.drawGeneratedCurve(shader);
    }
}

void CubicBezierSequence::drawPreview(Shader& shader) const
{
    for (const auto& curve : curves)
    {
        curve.drawControlPointsPreview(shader);
        curve.drawGeneratedCurvePreview(shader);
    }
}