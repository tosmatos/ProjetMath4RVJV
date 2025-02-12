#pragma once

// I needed to make a header for this Type enum specifically because I'd get
// circular references in PolyBuilder.h and Polygon.h otherwise
namespace PolyBuilder {
    enum Type { POLYGON, WINDOW };
}