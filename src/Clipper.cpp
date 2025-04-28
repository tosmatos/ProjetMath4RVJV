#include "Clipper.h"

#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath> // for fabs

namespace Clipper
{
    // Structure to hold parameter range for line clipping
    struct ParamRange
    {
        float tEnter = 0.0f;
        float tLeave = 1.0f;
    };

    // Helper: Dot product in 2D
    static float dot2D(float x1, float y1, float x2, float y2)
    {
        return x1 * x2 + y1 * y2;
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
    static bool is_inside(Vertex point, Vertex a, Vertex b, bool isWindowClockwise)
    {
        float crossProduct = (cross2D(a - b, point) + cross2D(b, a));
        if (isWindowClockwise)
            return crossProduct > 0.0f;
        else
            return crossProduct < 0.0f;
    }

    // Get intersection point
    static Vertex intersection(Vertex a1, Vertex a2, Vertex b1, Vertex b2)
    {
        return ((b1 - b2) * cross2D(a1, a2) - (a1 - a2) * cross2D(b1, b2)) * (1.0f / cross2D(a1 - a2, b1 - b2));
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
        if (isWindowCW)
        {
            nx = ey;
            ny = -ex;
        }
        else
        {
            nx = -ey;
            ny = ex;
        }

        float wx = A.x - edgeP1.x;
        float wy = A.y - edgeP1.y;

        float nDotD = dot2D(nx, ny, dx, dy);
        float nDotW = dot2D(nx, ny, wx, wy);

        if (fabs(nDotD) < 1e-7f)
        {
            // Line is parallel to the edge.
            if (nDotW < 0)
            {
                pr.tEnter = 1.0f;
                pr.tLeave = 0.0f;
            }
            return;
        }

        float t = -nDotW / nDotD;
        if (nDotD > 0)
        {
            if (t > pr.tEnter)
                pr.tEnter = t;
        }
        else
        {
            if (t < pr.tLeave)
                pr.tLeave = t;
        }
    }

    // Updated Cyrus–Beck polygon clipping function.
    // Determines the window's orientation and then clips each edge of the subject polygon.
    Polygon clipPolygonCyrusBeck(const Polygon& subject, const Polygon& windowPolygon)
    {
        const auto& subjVerts = subject.getVertices();
        const auto& winVerts = windowPolygon.getVertices();

        // Early exit if subject is empty or window is invalid.
        if (subjVerts.empty() || winVerts.size() < 3)
            return subject;

        bool isWindowCW = windowPolygon.isClockwise();

        Polygon resultPoly;
        resultPoly.type = subject.type;

        // Process each edge of the subject polygon.
        for (size_t i = 0; i < subjVerts.size(); i++)
        {
            const Vertex& A = subjVerts[i];
            const Vertex& B = subjVerts[(i + 1) % subjVerts.size()];

            ParamRange pr;

            // Clip this segment against every edge of the window polygon.
            for (size_t j = 0; j < winVerts.size(); j++)
            {
                size_t k = (j + 1) % winVerts.size();
                clipLineCyrusBeck(A, B, winVerts[j], winVerts[k], pr, isWindowCW);
                if (pr.tEnter > pr.tLeave)
                {
                    break;
                }
            }

            // If the parameter range is valid, compute the clipped segment endpoints.
            if (pr.tEnter <= pr.tLeave)
            {
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

    Polygon clipPolygonSutherlandHodgman(const Polygon& subject, const Polygon& windowPolygon)
    {
        Polygon clippedPoly = Polygon();

        if (subject.getVertices().size() < 3 || windowPolygon.getVertices().size() < 3)
            return clippedPoly; // Early exit

        std::vector<Vertex> windowVertices = windowPolygon.getVertices();
        std::vector<Vertex> buildingVertices{ subject.getVertices().begin(), subject.getVertices().end() };
        std::vector<Vertex> input;

        Vertex p1 = windowVertices[windowVertices.size() - 1];

        bool isWindowClockwise = windowPolygon.isClockwise();

        for (Vertex p2 : windowVertices)
        {
            input.clear();
            input.insert(input.end(), buildingVertices.begin(), buildingVertices.end());

            // Check if input is empty before accessing it
            if (input.empty())
            {
                buildingVertices.clear(); // Clear buildingVertices as it's no longer needed for this edge
                p1 = p2;
                continue; // Skip to the next window edge if input is empty
            }

            Vertex previousVertex = input[input.size() - 1];

            // Use a temporary vector for the next building vertices
            // Necessary as Sutherland-Hodgman is sequential
            std::vector<Vertex> nextBuildingVertices;

            for (Vertex currentVertex : input)
            {
                if (is_inside(currentVertex, p1, p2, isWindowClockwise))
                {
                    if (!is_inside(previousVertex, p1, p2, isWindowClockwise))
                    {
                        nextBuildingVertices.push_back(intersection(p1, p2, previousVertex, currentVertex));
                    }
                    nextBuildingVertices.push_back(currentVertex);
                }
                else if (is_inside(previousVertex, p1, p2, isWindowClockwise))
                {
                    nextBuildingVertices.push_back(intersection(p1, p2, previousVertex, currentVertex));
                }

                previousVertex = currentVertex;
            }

            buildingVertices = nextBuildingVertices;
            p1 = p2;
        }

        clippedPoly.setVertices(buildingVertices);
        clippedPoly.updateBuffers();
        return clippedPoly;
    }

    // Helper function to check if a point is inside a triangle
    static bool isPointInsideTriangle(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c)
    {
        float cross1 = cross2D(b - a, p - a);
        float cross2 = cross2D(c - b, p - b);
        float cross3 = cross2D(a - c, p - c);

        bool hasNeg = (cross1 < 0) || (cross2 < 0) || (cross3 < 0);
        bool hasPos = (cross1 > 0) || (cross2 > 0) || (cross3 > 0);

        return !(hasNeg && hasPos);
    }

    // Helper function to check if a vertex is an ear
    static bool isEar(const Vertex& prev, const Vertex& curr, const Vertex& next, const std::vector<Vertex>& polygon)
    {
        // Check if the triangle is convex
        float cross = cross2D(curr - prev, next - curr);
        if (cross <= 0) // Not convex (concave or colinear)
            return false;

        // Check if any other vertex is inside the triangle
        for (const Vertex& v : polygon)
        {
            if (&v == &prev || &v == &curr || &v == &next)
                continue;

            if (isPointInsideTriangle(v, prev, curr, next))
                return false;
        }

        return true;
    }

    // Ear Cutting algorithm to decompose a concave polygon into triangles
    std::vector<Polygon> earCutting(const Polygon& concavePolygon)
    {
        std::vector<Polygon> triangles;
        std::vector<Vertex> vertices = concavePolygon.getVertices();

        // Early exit if the polygon is already a triangle
        if (vertices.size() == 3)
        {
            triangles.push_back(concavePolygon);
            return triangles;
        }

        // Make sure the polygon is counterclockwise
        if (concavePolygon.isClockwise())
        {
            std::reverse(vertices.begin(), vertices.end());
        }

        // Iterate until the polygon is fully decomposed
        while (vertices.size() >= 3)
        {
            bool earFound = false;

            for (size_t i = 0; i < vertices.size(); i++)
            {
                size_t prevIndex = (i == 0) ? vertices.size() - 1 : i - 1;
                size_t nextIndex = (i + 1) % vertices.size();

                const Vertex& prev = vertices[prevIndex];
                const Vertex& curr = vertices[i];
                const Vertex& next = vertices[nextIndex];

                if (isEar(prev, curr, next, vertices))
                {
                    // Create a triangle from the ear
                    Polygon triangle;
                    triangle.addVertex(prev);
                    triangle.addVertex(curr);
                    triangle.addVertex(next);
                    triangles.push_back(triangle);

                    // Remove the ear vertex from the polygon
                    vertices.erase(vertices.begin() + i);
                    earFound = true;
                    break;
                }
            }

            // If no ear is found, the polygon may be degenerate
            if (!earFound)
            {
                std::cerr << "Error: No ear found. Polygon may be degenerate." << std::endl;
                break;
            }
        }

        return triangles;
    }
}