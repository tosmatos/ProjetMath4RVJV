#include "MathUtils.h"

#include <cmath>

namespace MathUtils
{
    long long combinations(int n, int k)
    {
        // Handle invalid inputs
        if (k < 0 || k > n)
        {
            return 0;
        }
        // C(n, 0) and C(n, n) are always 1
        if (k == 0 || k == n)
        {
            return 1;
        }
        // Use symmetry: C(n, k) = C(n, n-k) to reduce calculations
        // Choisir k éléments parmi n est la même chose que choisir de ne pas choisir n−k éléments parmi n.
        if (k > n / 2)
        {
            k = n - k;
        }
        // Calculate the binomial coefficient iteratively
        long long res = 1;
        for (int i = 1; i <= k; ++i)
        {
            res = res * (n - i + 1) / i;
        }
        return res;
    }

    float squaredDistance(const Vertex& v1, const Vertex& v2)
    {
        return (v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y);
    }

    int orientation(const Vertex& p, const Vertex& q, const Vertex& r)
    {
        float value = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
        if (value == 0) return 0; // Collinear
        return (value > 0) ? 1 : 2; // Counter clockwise : clockwise
    }

    float dot2D(float x1, float y1, float x2, float y2)
    {
        return x1 * x2 + y1 * y2;
    }

    float dot2D(Vertex a, Vertex b)
    {
        return a.x * b.x + a.y * b.y;
    }

    float cross2D(float x1, float y1, float x2, float y2)
    {
        return x1 * y2 - x2 * y1;
    }

    float cross2D(Vertex a, Vertex b)
    {
        return a.x * b.y - b.x * a.y;
    }

    bool is_inside(Vertex point, Vertex a, Vertex b, bool isWindowClockwise)
    {
        float crossProduct = (cross2D(a - b, point) + cross2D(b, a));
        if (isWindowClockwise)
            return crossProduct > 0.0f;
        else
            return crossProduct < 0.0f;
    }

    Vertex intersection(Vertex a1, Vertex a2, Vertex b1, Vertex b2)
    {
        return ((b1 - b2) * cross2D(a1, a2) - (a1 - a2) * cross2D(b1, b2)) * (1.0f / cross2D(a1 - a2, b1 - b2));
    }

    bool lineSegmentsIntersect(const Vertex& segmentA_start, const Vertex& segmentA_end,
        const Vertex& segmentB_start, const Vertex& segmentB_end, Vertex& intersectionPoint)
    {
        // Calculate the direction vectors of our two line segments
        Vertex directionA(segmentA_end.x - segmentA_start.x, segmentA_end.y - segmentA_start.y);
        Vertex directionB(segmentB_end.x - segmentB_start.x, segmentB_end.y - segmentB_start.y);

        // We'll use the cross product to determine if the lines are parallel
        // TODO : Cross product exists in Clipper.cpp. Find a way to make it clean and portable
        float crossProduct = directionA.x * directionB.y - directionA.y * directionB.x;

        // If the cross product is nearly zero, the lines are parallel or collinear
        // We use a small epsilon value to account for floating point precision
        if (std::abs(crossProduct) < 1e-6)
            return false;  // Parallel lines don't intersect

        // Calculate the vector from start of segment A to start of segment B
        Vertex startDifference(segmentB_start.x - segmentA_start.x, segmentB_start.y - segmentA_start.y);

        // Calculate how far along segment A the intersection occurs (from 0 to 1)
        // We use the formula: t = (startDifference × directionB) / (directionA × directionB)
        // Where × is the 2D cross product
        float intersectionRatioA = (startDifference.x * directionB.y - startDifference.y * directionB.x) / crossProduct;

        // Calculate how far along segment B the intersection occurs (from 0 to 1)
        // We use the formula: u = (startDifference × directionA) / (directionA × directionB)
        float intersectionRatioB = (startDifference.x * directionA.y - startDifference.y * directionA.x) / crossProduct;

        // Check if the intersection point is within both segments
        // For the intersection to be on both segments, both ratios must be between 0 and 1
        if (intersectionRatioA >= 0 && intersectionRatioA <= 1 && intersectionRatioB >= 0 && intersectionRatioB <= 1)
        {
            // Calculate the intersection point using the ratio along segment A
            // Doesn't matter if we use intersectionRatioA or B for getting the point
            intersectionPoint.x = segmentA_start.x + intersectionRatioA * directionA.x;
            intersectionPoint.y = segmentA_start.y + intersectionRatioA * directionA.y;
            return true;  // We found an intersection!
        }

        return false;
    }
}