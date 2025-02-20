#include "Clipper.h"
#include <iostream>
#include <vector>
#include <cmath> // for fabs

// Structure to hold parameter range for line clipping
struct ParamRange {
    float tEnter = 0.0f;
    float tLeave = 1.0f;
};

// Helper: Dot product in 2D
static float dot2D(float x1, float y1, float x2, float y2)
{
    return x1 * x2 + y1 * y2;
}

// The extra parameter 'isWindowCW' tells whether the window polygon is clockwise.
// For clockwise windows, the outward normal is (ey, -ex); for counterclockwise, it's reversed.
static void clipLineCyrusBeck(const Vertex& A,
                              const Vertex& B,
                              const Vertex& edgeP1,
                              const Vertex& edgeP2,
                              ParamRange& pr,
                              bool isWindowCW)
{
    float dx = B.x - A.x;
    float dy = B.y - A.y;

    float ex = edgeP2.x - edgeP1.x;
    float ey = edgeP2.y - edgeP1.y;

    // Compute the outward normal based on window orientation
    float nx, ny;
    if (isWindowCW) {
        nx = ey;
        ny = -ex;
    } else {
        nx = -ey;
        ny = ex;
    }

    float wx = A.x - edgeP1.x;
    float wy = A.y - edgeP1.y;

    float nDotD = dot2D(nx, ny, dx, dy);
    float nDotW = dot2D(nx, ny, wx, wy);

    if (fabs(nDotD) < 1e-7f) {
        // Line is parallel to the edge.
        if (nDotW < 0) {
            pr.tEnter = 1.0f;
            pr.tLeave = 0.0f;
        }
        return;
    }

    float t = - nDotW / nDotD;
    if (nDotD > 0) {
        if (t > pr.tEnter)
            pr.tEnter = t;
    } else {
        if (t < pr.tLeave)
            pr.tLeave = t;
    }
}

// Helper: Compute the signed area of a polygon.
// A positive area means the vertices are in counterclockwise order.
static float computePolygonArea(const std::vector<Vertex>& verts)
{
    float area = 0.0f;
    size_t n = verts.size();
    for (size_t i = 0; i < n; i++) {
        const Vertex& current = verts[i];
        const Vertex& next = verts[(i + 1) % n];
        area += current.x * next.y - next.x * current.y;
    }
    return area * 0.5f;
}

// Updated Cyrus–Beck polygon clipping function.
// Determines the window's orientation and then clips each edge of the subject polygon.
Polygon clipPolygonCyrusBeck(const Polygon& subject, const Polygon& windowPolygon)
{
    const auto& subjVerts = subject.getVertices();
    const auto& winVerts  = windowPolygon.getVertices();

    // Early exit if subject is empty or window is invalid.
    if (subjVerts.empty() || winVerts.size() < 3)
        return subject;

    // Determine window polygon orientation using signed area.
    // If area is negative, the window is clockwise.
    float area = computePolygonArea(winVerts);
    bool isWindowCW = (area < 0.0f);

    Polygon resultPoly;
    resultPoly.type = subject.type;

    // Process each edge of the subject polygon.
    for (size_t i = 0; i < subjVerts.size(); i++)
    {
        const Vertex& A = subjVerts[i];
        const Vertex& B = subjVerts[(i + 1) % subjVerts.size()];

        ParamRange pr;

        // Clip this segment against every edge of the window polygon.
        for (size_t j = 0; j < winVerts.size(); j++) {
            size_t k = (j + 1) % winVerts.size();
            clipLineCyrusBeck(A, B, winVerts[j], winVerts[k], pr, isWindowCW);
            if (pr.tEnter > pr.tLeave) {
                break;
            }
        }

        // If the parameter range is valid, compute the clipped segment endpoints.
        if (pr.tEnter <= pr.tLeave) {
            float dx = B.x - A.x;
            float dy = B.y - A.y;

            float Ax_cl = A.x + pr.tEnter * dx;
            float Ay_cl = A.y + pr.tEnter * dy;
            float Bx_cl = A.x + pr.tLeave * dx;
            float By_cl = A.y + pr.tLeave * dy;

            if (pr.tEnter >= 0.f && pr.tEnter <= 1.f)
                resultPoly.addVertex(Ax_cl, Ay_cl);
            if (pr.tLeave >= 0.f && pr.tLeave <= 1.f)
                resultPoly.addVertex(Bx_cl, By_cl);
        }
    }

    resultPoly.updateBuffers();
    return resultPoly;
}
