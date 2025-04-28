#pragma once

// I needed to make a header for this PolyType enum specifically because I'd get
// circular references in PolyBuilder.h and Polygon.h otherwise
enum PolyType {
    POLYGON,
    WINDOW,
    CLIPPED_CYRUS_BECK,
    CLIPPED_SUTHERLAND_HODGMAN
};