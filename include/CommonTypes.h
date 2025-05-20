#pragma once

// For some functions that differentiate between all of these
enum PolyType {
    POLYGON,
    WINDOW,
    CLIPPED_CYRUS_BECK,
    CLIPPED_SUTHERLAND_HODGMAN,
    BEZIER_CURVE,
    CONVEX_HULL,
};

// For shapes transormation handling
enum TransformationType
{
	TRANSLATE,
	SCALE,
	ROTATE,
	SHEAR
};

// For avoiding having boolean flags
enum ShapeType {
    SHAPE_POLYGON,
    SHAPE_BEZIER,
    SHAPE_BEZIER_SEQUENCE
};