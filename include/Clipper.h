//
// Created by kouih on 16/02/2025.
//

#ifndef CLIPPER_H
#define CLIPPER_H

#pragma once
#include "Polygon.h"

Polygon clipPolygonCyrusBeck(const Polygon& subject, const Polygon& windowPolygon);
#endif //CLIPPER_H
