#pragma once

#include "Vertex.h"

namespace MathUtils
{
	// Helper function to calculate binomial coefficient C(n, k)
	// This is the mathematical representation of the numbers in Pascal's triangle.
	// C(n, k) = n! / (k! * (n-k)!)
	// See Maths Chap 1 Courbes - 3 Base de bernstein
	long long combinations(int n, int k);
	// Helper to get squared distance, to check if collinear
	float squaredDistance(const Vertex& v1, const Vertex& v2);
	// Helper to get relative orientation orientation
	// Basically : does traversing p to q and then q to r make a left turn, right turn or collinear 
	// 0 = Collinear, 1 = Counter-Clockwise (left turn), 2 = Clockwise (right turn)
	int orientation(const Vertex& p, const Vertex& q, const Vertex& r);
    // Vector Dot product in 2D
    float dot2D(float x1, float y1, float x2, float y2);
    // Vector Dot product in 2D
    float dot2D(Vertex a, Vertex b);
    // a.k.a. Produit vectoriel
    float cross2D(float x1, float y1, float x2, float y2);
    // a.k.a. Produit vectoriel
    float cross2D(Vertex a, Vertex b);
    // Check if point is on the left side of an edge
    bool is_inside(Vertex point, Vertex a, Vertex b, bool isWindowClockwise);
    // Get intersection point
    Vertex intersection(Vertex a1, Vertex a2, Vertex b1, Vertex b2);
    // Returns true if two line segments intersect, and intersection point gets set
    bool lineSegmentsIntersect(const Vertex& segmentA_start, const Vertex& segmentA_end,
        const Vertex& segmentB_start, const Vertex& segmentB_end, Vertex& intersectionPoint);
}