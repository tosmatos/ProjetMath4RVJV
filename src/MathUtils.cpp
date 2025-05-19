#include "MathUtils.h"

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
}