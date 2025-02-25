#pragma once
#include "Polygon.h"

namespace Clipper
{
	Polygon clipPolygonCyrusBeck(const Polygon& subject, const Polygon& windowPolygon);
	Polygon clipPolygonSutherlandHodgman(const Polygon& subject, const Polygon& windowPolygon);
	std::vector<Polygon> earCutting(const Polygon& concavePolygon);
}
