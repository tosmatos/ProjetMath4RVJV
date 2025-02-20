//
// Created by kouih on 16/02/2025.
//
#include "Clipper.h"
#include <iostream>
#include <vector>

struct ParamRange {
    float tEnter = 0.0f;
    float tLeave = 1.0f;
};

static float dot2D(float x1, float y1, float x2, float y2)
{
    return x1*x2 + y1*y2;
}

static float dot2D(Vertex a, Vertex b)
{
    return a.x * b.x + a.y * b.y;
}

// a.k.a. Produit vectoriel
static float cross2D(float x1, float y1, float x2, float y2)
{
    return x1 * y2 - x2 * y1;
}

static float cross2D(Vertex a, Vertex b)
{
    return a.x * b.y - b.x * a.y;
}

// Check if point is on the left side of an edge
static bool is_inside(Vertex point, Vertex a, Vertex b)
{
    return (cross2D(a - b, point) + cross2D(b, a)) < 0.0f;
}

// Get intersection point
static Vertex intersection(Vertex a1, Vertex a2, Vertex b1, Vertex b2)
{
    return ((b1 - b2) * cross2D(a1, a2) - (a1 - a2) * cross2D(b1, b2)) * (1.0f / cross2D(a1 - a2, b1 - b2));
}

static void clipLineCyrusBeck(const Vertex& A,
                              const Vertex& B,
                              const Vertex& edgeP1,
                              const Vertex& edgeP2,
                              ParamRange& pr)
{
    float dx = B.x - A.x;
    float dy = B.y - A.y;

    float ex = edgeP2.x - edgeP1.x;
    float ey = edgeP2.y - edgeP1.y;

    float nx = ey;       // outward normal X
    float ny = -ex;      // outward normal Y

    float wx = A.x - edgeP1.x;
    float wy = A.y - edgeP1.y;

    float nDotD = dot2D(nx, ny, dx, dy);
    float nDotW = dot2D(nx, ny, wx, wy);

    if (fabs(nDotD) < 1e-7f) {
        if (nDotW < 0) {
            pr.tEnter = 1.0f;
            pr.tLeave = 0.0f;
        }
        return;
    }

    float t = - nDotW / nDotD;
    if (nDotD > 0) {
        if (t > pr.tEnter) pr.tEnter = t;
    } else {
        if (t < pr.tLeave) pr.tLeave = t;
    }
}

Polygon clipPolygonCyrusBeck(const Polygon& subject, const Polygon& windowPolygon)
{
    const auto& subjVerts = subject.getVertices();
    const auto& winVerts  = windowPolygon.getVertices();
    // Early exit checks
    if (subjVerts.empty() || winVerts.size() < 3)
        return subject; // no change or invalid window

    Polygon resultPoly;
    resultPoly.type = subject.type;


    for (size_t i = 0; i < subjVerts.size(); i++)
    {
        const Vertex& A = subjVerts[i];
        const Vertex& B = subjVerts[(i+1) % subjVerts.size()];

        ParamRange pr;

        for (size_t j = 0; j < winVerts.size(); j++) {
            size_t k = (j + 1) % winVerts.size();
            clipLineCyrusBeck(A, B, winVerts[j], winVerts[k], pr);
            if (pr.tEnter > pr.tLeave) {
                break;
            }
        }

        if (pr.tEnter <= pr.tLeave) {
            float dx = (B.x - A.x);
            float dy = (B.y - A.y);

            // Start
            float Ax_cl = A.x + pr.tEnter * dx;
            float Ay_cl = A.y + pr.tEnter * dy;
            // End
            float Bx_cl = A.x + pr.tLeave * dx;
            float By_cl = A.y + pr.tLeave * dy;

            if (pr.tEnter >= 0.f && pr.tEnter <= 1.f) {
                resultPoly.addVertex(Ax_cl, Ay_cl);
            }
            // Push end
            if (pr.tLeave >= 0.f && pr.tLeave <= 1.f) {
                resultPoly.addVertex(Bx_cl, By_cl);
            }
        }
    }

    resultPoly.updateBuffers();
    return resultPoly;
}

Polygon clipPolygonSutherlandHodgman(const Polygon& subject, const Polygon& windowPolygon)
{

    return subject;
}