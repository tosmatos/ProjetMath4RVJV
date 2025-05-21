#include "PolyBuilder.h"
#include "MathUtils.h"

#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <iostream>
#include <string>
#include <functional>

using namespace MathUtils;

void PolyBuilder::startPolygon(PolyType type)
{
	polyType = type;
	buildingShape = true;
	tempPolygon = Polygon();
}

void PolyBuilder::startBezierCurve()
{
	tempBezier = Bezier();
	buildingShape = true;
	toggleBezierMode();
}

void PolyBuilder::removeFinishedBezier(size_t index)
{
	if (index < finishedBeziers.size())
		finishedBeziers.erase(finishedBeziers.begin() + index);
}

void PolyBuilder::swapBezierAlgorithm(size_t index)
{
	if (index < finishedBeziers.size())
		finishedBeziers[index].swapAlgorithm();
}

void PolyBuilder::incrementBezierStepSize(size_t index)
{
	if (index < finishedBeziers.size())
		finishedBeziers[index].incrementStepSize();
}

void PolyBuilder::decrementBezierStepSize(size_t index)
{
	if (index < finishedBeziers.size())
		finishedBeziers[index].decrementStepSize();
}

void PolyBuilder::toggleHullDisplay(size_t index)
{
	if (index < finishedBeziers.size())
		finishedBeziers[index].toggleConvexHullDisplay();
}

void PolyBuilder::swapSequenceAlgorithm(size_t index)
{
	if (index < finishedSequences.size())
		finishedSequences[index].swapAlgorithm();
}

void PolyBuilder::incrementSequenceStepSize(size_t index)
{
	if (index < finishedSequences.size())
		finishedSequences[index].incrementStepSize();
}

void PolyBuilder::decrementSequenceStepSize(size_t index)
{
	if (index < finishedSequences.size())
		finishedSequences[index].decrementStepSize();
}

void PolyBuilder::removeFinishedSequence(size_t index)
{
	if (index < finishedSequences.size())
		finishedSequences.erase(finishedSequences.begin() + index);
}

void PolyBuilder::duplicateControlPoint(int shapeIndex, int vertexIndex)
{
	if (shapeIndex < finishedBeziers.size())
		finishedBeziers[shapeIndex].duplicateControlPoint(vertexIndex);
}

/* Why translation can be done with one matrix while others cannot:
 * Translation is an affine transformation, while scale, rotation and shear are linear transformations.
 *
 * Linear transformations have two critical properties:
 * 1. They preserve the origin (0,0) -> (0,0)
 * 2. They preserve vector addition and scalar multiplication
 *
 * For operations like scaling and rotation around a shape's center, we need to:
 *   a) Translate to origin
 *   b) Apply the linear transformation
 *   c) Translate back to original position
 *
 * This connects to homogeneous coordinates where we distinguish:
 * - Points: positions in space (x,y,1)
 * - Vectors: directions with magnitude (x,y,0)
 *
 * In homogeneous coordinates, a 3x3 transformation matrix has form:
 * | a b tx |
 * | c d ty |
 * | 0 0 1  |
 *
 * - For points (w=1): Both the linear part (a,b,c,d) AND translation (tx,ty) apply
 * - For vectors (w=0): Only the linear part applies, translation is ignored
 *
 * This is why we need to track original vertices for linear transformations—to prevent
 * unwanted compounding effects when applying multiple transformations.
 */

void PolyBuilder::translate(int shapeIndex, ShapeType shapeType, float deltaX, float deltaY)
{
	foundIntersectionsText.clear();
	intersections.clear();
	Matrix3x3 translationMatrix = createTranslationMatrix(deltaX, deltaY);

	// Special handling for Bézier sequences since they contain multiple collections of vertices
	if (shapeType == SHAPE_BEZIER_SEQUENCE) {
		if (shapeIndex >= 0 && shapeIndex < finishedSequences.size()) {
			// Create a completely fresh copy
			CubicBezierSequence originalSequence = finishedSequences[shapeIndex];

			// Work on the copy, transform all points
			std::vector<Bezier>& curves = originalSequence.getCurves();
			for (auto& curve : curves) {
				std::vector<Vertex> points = curve.getControlPoints();
				for (auto& point : points) {
					Vertex transformedPoint = multiplyMatrixVertex(translationMatrix, point);
					point.x = transformedPoint.x;
					point.y = transformedPoint.y;
				}
				curve.setControlPoints(points);
				curve.generateCurve();
			}

			// Apply constraints and update buffers
			originalSequence.enforceConstraints();
			for (auto& curve : curves) {
				curve.updateBuffers();
			}

			// Replace the original with our transformed copy
			finishedSequences[shapeIndex] = originalSequence;
		}
		return;
	}

	// Standard handling for the other shape types
	std::vector<Vertex> vertices;

	if (shapeType == SHAPE_POLYGON) {
		if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size()) {
			Polygon& poly = finishedPolygons[shapeIndex];
			vertices = poly.getVertices();
		}
	}
	else if (shapeType == SHAPE_BEZIER) {
		if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size()) {
			Bezier& bezier = finishedBeziers[shapeIndex];
			vertices = bezier.getControlPoints();
		}
	}

	// Transform the vertices
	for (Vertex& vertex : vertices) {
		Vertex transformedPoint = multiplyMatrixVertex(translationMatrix, vertex);
		vertex.x = transformedPoint.x;
		vertex.y = transformedPoint.y;
	}

	// Update the shape
	if (shapeType == SHAPE_POLYGON) {
		if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size()) {
			Polygon& poly = finishedPolygons[shapeIndex];
			poly.setVertices(vertices);
			poly.updateBuffers();
		}
	}
	else if (shapeType == SHAPE_BEZIER) {
		if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size()) {
			Bezier& bezier = finishedBeziers[shapeIndex];
			bezier.setControlPoints(vertices);
			bezier.generateCurve();
			bezier.updateBuffers();
		}
	}
}

void PolyBuilder::translateVertex(int shapeIndex, int vertexIndex, ShapeType shapeType, float deltaX, float deltaY)
{
	foundIntersectionsText.clear();
	intersections.clear();
	Matrix3x3 translationMatrix = createTranslationMatrix(deltaX, deltaY);

	if (shapeType == SHAPE_BEZIER_SEQUENCE)
	{
		if (shapeIndex < 0 || shapeIndex >= finishedSequences.size())
		{
			std::cerr << "Error: Invalid sequence index " << shapeIndex << std::endl;
			return;
		}
		// Create a complete copy of the sequence to work with
		CubicBezierSequence transformedSequence = finishedSequences[shapeIndex];
		// Determine which curve and which point within that curve
		int curveIndex = vertexIndex / 4;  // Integer division to get curve index
		int pointIndexInCurve = vertexIndex % 4;  // Remainder gives point index within curve
		// Check if this point is constrained
		if (transformedSequence.isConstrainedPoint(curveIndex, pointIndexInCurve))
		{
			std::cout << "Cannot move constrained point in curve " << curveIndex
				<< " at position " << pointIndexInCurve << std::endl;
			return;
		}

		// Get the curve that contains our point from our copy
		std::vector<Bezier>& curves = transformedSequence.getCurves();
		if (curveIndex >= 0 && curveIndex < curves.size())
		{
			Bezier& curve = curves[curveIndex];
			std::vector<Vertex> controlPoints = curve.getControlPoints();
			if (pointIndexInCurve >= 0 && pointIndexInCurve < controlPoints.size())
			{
				// Apply translation to the specific point
				Vertex& point = controlPoints[pointIndexInCurve];
				Vertex transformedPoint = multiplyMatrixVertex(translationMatrix, point);
				point.x = transformedPoint.x;
				point.y = transformedPoint.y;

				// Update the curve with the modified control points
				curve.setControlPoints(controlPoints);

				// Handle closed sequence synchronization
				if (transformedSequence.getIsClosed())
				{
					// If we're moving the first point of the first curve
					if (curveIndex == 0 && pointIndexInCurve == 0)
					{
						// Update the end point of the last curve to match
						int lastCurveIndex = curves.size() - 1;
						std::vector<Vertex> lastPoints = curves[lastCurveIndex].getControlPoints();
						lastPoints[3] = point; // Direct assignment to ensure exact match
						curves[lastCurveIndex].setControlPoints(lastPoints);
						curves[lastCurveIndex].generateCurve();
						curves[lastCurveIndex].updateBuffers();
					}
					// If we're moving the last point of the last curve
					else if (curveIndex == curves.size() - 1 && pointIndexInCurve == 3)
					{
						// Update the first point of the first curve to match
						std::vector<Vertex> firstPoints = curves[0].getControlPoints();
						firstPoints[0] = point; // Direct assignment to ensure exact match
						curves[0].setControlPoints(firstPoints);
						curves[0].generateCurve();
						curves[0].updateBuffers();
					}
				}

				curve.generateCurve();
				curve.updateBuffers();

				// Enforce continuity constraints across the sequence
				transformedSequence.enforceConstraints();

				// Now that all modifications are done on the copy, replace the original
				finishedSequences[shapeIndex] = transformedSequence;
			}
		}
		return;
	}

	std::vector<Vertex> vertices;
	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		Polygon& poly = finishedPolygons[shapeIndex];
		vertices = poly.getVertices();
		break;
	}
	case SHAPE_BEZIER:
	{
		Bezier& bezier = finishedBeziers[shapeIndex];
		vertices = bezier.getControlPoints();
		break;
	}
	}

	Vertex& vertex = vertices[vertexIndex];

	Vertex transformedPoint = multiplyMatrixVertex(translationMatrix, vertex);
	vertex.x = transformedPoint.x;
	vertex.y = transformedPoint.y;

	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		Polygon& poly = finishedPolygons[shapeIndex];
		poly.setVertices(vertices);
		poly.updateBuffers();
		break;
	}
	case SHAPE_BEZIER:
	{
		Bezier& bezier = finishedBeziers[shapeIndex];
		bezier.setControlPoints(vertices);
		bezier.generateCurve();
		bezier.updateBuffers();
		break;
	}
	}
}

void PolyBuilder::startTransformingShape(int shapeIndex, ShapeType shapeType)
{
	if (shapeType == SHAPE_BEZIER_SEQUENCE)
	{
		if (shapeIndex >= 0 && shapeIndex < finishedSequences.size())
		{
			// Store each curve's points separately
			for (const auto& curve : finishedSequences[shapeIndex].getCurves()) {
				sequenceTransformData.originalCurvePoints.push_back(
					curve.getControlPoints()
				);
			}

			isCurrentlyTransformingShape = true;
		}
	}
	else
	{
		if (shapeType == SHAPE_POLYGON)
		{
			if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size())
			{
				transformOriginalVertices = finishedPolygons[shapeIndex].getVertices();
				isCurrentlyTransformingShape = true;
			}
		}
		else if (shapeType == SHAPE_BEZIER)
		{
			if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size())
			{
				transformOriginalVertices = finishedBeziers[shapeIndex].getControlPoints();
				isCurrentlyTransformingShape = true;
				foundIntersectionsText.clear();
				intersections.clear();
			}
		}
	}
}

void PolyBuilder::stopTransformingShape()
{
	isCurrentlyTransformingShape = false;
	transformOriginalVertices.clear();
	sequenceTransformData.originalCurvePoints.clear();
}

void PolyBuilder::applyScaleFromOriginal(int shapeIndex, ShapeType shapeType, float totalScaleFactorX, float totalScaleFactorY)
{
	if (!isCurrentlyTransformingShape)
		return; // Not in a scaling operation or no original vertices

	if (shapeType == SHAPE_BEZIER_SEQUENCE)
	{
		if (sequenceTransformData.originalCurvePoints.empty() ||
			shapeIndex < 0 || shapeIndex >= finishedSequences.size())
			return;

		// Create a completely fresh copy of the original sequence
		CubicBezierSequence transformedSequence = finishedSequences[shapeIndex];

		// Get a reference to the curves in our copied sequence
		std::vector<Bezier>& curves = transformedSequence.getCurves();

		// Flatten all points to calculate center
		std::vector<Vertex> allPoints;
		for (const auto& curvePoints : sequenceTransformData.originalCurvePoints) {
			allPoints.insert(allPoints.end(), curvePoints.begin(), curvePoints.end());
		}

		Vertex center = calculateCenter(allPoints);

		// Create transformation matrices
		Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
		Matrix3x3 scalingMatrix = createScalingMatrix(totalScaleFactorX, totalScaleFactorY);
		Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
		Matrix3x3 finalMatrix = translateBack * scalingMatrix * translateToOrigin;

		// Transform and update each curve
		for (size_t i = 0; i < sequenceTransformData.originalCurvePoints.size() && i < curves.size(); i++) {
			std::vector<Vertex> transformedPoints = sequenceTransformData.originalCurvePoints[i];

			// Transform the points
			for (Vertex& point : transformedPoints) {
				point = multiplyMatrixVertex(finalMatrix, point);
			}

			// Update the curve in our copy
			curves[i].setControlPoints(transformedPoints);
			curves[i].generateCurve();
		}

		// Re-enforce constraints on our copy
		transformedSequence.enforceConstraints();

		// Update buffers in our copy
		for (auto& curve : curves) {
			curve.updateBuffers();
		}

		// Replace the original with our transformed copy
		finishedSequences[shapeIndex] = transformedSequence;

		return;
	}

	if (transformOriginalVertices.empty())
		return;

	std::vector<Vertex> newVertices = transformOriginalVertices; // Work on a copy of the original

	Vertex center = calculateCenter(transformOriginalVertices); // Center of the *original* shape

	Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
	Matrix3x3 scalingMatrix = createScalingMatrix(totalScaleFactorX, totalScaleFactorY);
	Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
	Matrix3x3 finalMatrix = translateBack * scalingMatrix * translateToOrigin;

	for (Vertex& vertex : newVertices)
	{
		// Apply the total transformation to each original vertex
		Vertex transformedPoint = multiplyMatrixVertex(finalMatrix, vertex);
		vertex.x = transformedPoint.x;
		vertex.y = transformedPoint.y;
	}

	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		if (shapeIndex >= 0 && shapeIndex < finishedPolygons.size())
		{
			Polygon& poly = finishedPolygons[shapeIndex];
			poly.setVertices(newVertices);
			poly.updateBuffers();
		}
		break;
	}
	case SHAPE_BEZIER:
	{
		if (shapeIndex >= 0 && shapeIndex < finishedBeziers.size())
		{
			Bezier& bezier = finishedBeziers[shapeIndex];
			bezier.setControlPoints(newVertices);
			bezier.generateCurve();
			bezier.updateBuffers();
		}
		break;
	}
	}
}

void PolyBuilder::applyRotationFromOriginal(int shapeIndex, ShapeType shapeType, float totalRotationAngle)
{
	if (!isCurrentlyTransformingShape)
		return; // Not in a scaling operation or no original vertices

	if (shapeType == SHAPE_BEZIER_SEQUENCE)
	{
		if (sequenceTransformData.originalCurvePoints.empty() ||
			shapeIndex < 0 || shapeIndex >= finishedSequences.size())
			return;

		// Create a completely fresh copy
		CubicBezierSequence transformedSequence = finishedSequences[shapeIndex];

		// Get a reference to the curves in our copied sequence
		std::vector<Bezier>& curves = transformedSequence.getCurves();

		// Flatten all points to calculate center
		std::vector<Vertex> allPoints;
		for (const auto& curvePoints : sequenceTransformData.originalCurvePoints) {
			allPoints.insert(allPoints.end(), curvePoints.begin(), curvePoints.end());
		}

		Vertex center = calculateCenter(allPoints);

		// Create transformation matrices
		Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
		Matrix3x3 rotationMatrix = createRotationMatrix(totalRotationAngle);
		Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
		Matrix3x3 finalMatrix = translateBack * rotationMatrix * translateToOrigin;

		// Transform and update each curve
		for (size_t i = 0; i < sequenceTransformData.originalCurvePoints.size() && i < curves.size(); i++) {
			std::vector<Vertex> transformedPoints = sequenceTransformData.originalCurvePoints[i];

			// Transform the points
			for (Vertex& point : transformedPoints) {
				point = multiplyMatrixVertex(finalMatrix, point);
			}

			// Update the curve in our copy
			curves[i].setControlPoints(transformedPoints);
			curves[i].generateCurve();
		}

		// Re-enforce constraints on our copy
		transformedSequence.enforceConstraints();

		// Update buffers in our copy
		for (auto& curve : curves) {
			curve.updateBuffers();
		}

		// Replace the original with our transformed copy
		finishedSequences[shapeIndex] = transformedSequence;

		return;
	}

	if (transformOriginalVertices.empty())
		return;

	std::vector<Vertex> newVertices = transformOriginalVertices; // Work on a copy of the original

	Vertex center = calculateCenter(transformOriginalVertices); // Center of the *original* shape

	Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
	Matrix3x3 rotationMatrix = createRotationMatrix(totalRotationAngle);
	Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
	Matrix3x3 finalMatrix = translateBack * rotationMatrix * translateToOrigin;

	for (Vertex& vertex : newVertices)
	{
		Vertex transformedPoint = multiplyMatrixVertex(finalMatrix, vertex);
		vertex.x = transformedPoint.x;
		vertex.y = transformedPoint.y;
	}

	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		Polygon& poly = finishedPolygons[shapeIndex];
		poly.setVertices(newVertices);
		poly.updateBuffers();
		break;
	}
	case SHAPE_BEZIER:
	{
		Bezier& bezier = finishedBeziers[shapeIndex];
		bezier.setControlPoints(newVertices);
		bezier.generateCurve();
		bezier.updateBuffers();
		break;
	}
	}
}

void PolyBuilder::applyShearFromOriginal(int shapeIndex, ShapeType shapeType, float totalShearX, float totalShearY)
{
	if (!isCurrentlyTransformingShape)
		return; // Not in a scaling operation

	if (shapeType == SHAPE_BEZIER_SEQUENCE)
	{
		if (sequenceTransformData.originalCurvePoints.empty() ||
			shapeIndex < 0 || shapeIndex >= finishedSequences.size())
			return;

		// Create a completely fresh copy
		CubicBezierSequence transformedSequence = finishedSequences[shapeIndex];

		// Get a reference to the curves in our copied sequence
		std::vector<Bezier>& curves = transformedSequence.getCurves();

		// Flatten all points to calculate center
		std::vector<Vertex> allPoints;
		for (const auto& curvePoints : sequenceTransformData.originalCurvePoints) {
			allPoints.insert(allPoints.end(), curvePoints.begin(), curvePoints.end());
		}

		Vertex center = calculateCenter(allPoints);

		// Create transformation matrices
		Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
		Matrix3x3 shearingMatrix = createShearingMatrix(totalShearX, totalShearY);
		Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
		Matrix3x3 finalMatrix = translateBack * shearingMatrix * translateToOrigin;

		// Transform and update each curve
		for (size_t i = 0; i < sequenceTransformData.originalCurvePoints.size() && i < curves.size(); i++) {
			std::vector<Vertex> transformedPoints = sequenceTransformData.originalCurvePoints[i];

			// Transform the points
			for (Vertex& point : transformedPoints) {
				point = multiplyMatrixVertex(finalMatrix, point);
			}

			// Update the curve in our copy
			curves[i].setControlPoints(transformedPoints);
			curves[i].generateCurve();
		}

		// Re-enforce constraints on our copy
		transformedSequence.enforceConstraints();

		// Update buffers in our copy
		for (auto& curve : curves) {
			curve.updateBuffers();
		}

		// Replace the original with our transformed copy
		finishedSequences[shapeIndex] = transformedSequence;

		return;
	}

	if (transformOriginalVertices.empty())
		return;

	std::vector<Vertex> newVertices = transformOriginalVertices; // Work on a copy of the original

	Vertex center = calculateCenter(transformOriginalVertices); // Center of the *original* shape

	Matrix3x3 translateToOrigin = createTranslationMatrix(-center.x, -center.y);
	Matrix3x3 shearingMatrix = createShearingMatrix(totalShearX, totalShearY);
	Matrix3x3 translateBack = createTranslationMatrix(center.x, center.y);
	Matrix3x3 finalMatrix = translateBack * shearingMatrix * translateToOrigin;

	for (Vertex& vertex : newVertices)
	{
		Vertex transformedPoint = multiplyMatrixVertex(finalMatrix, vertex);
		vertex.x = transformedPoint.x;
		vertex.y = transformedPoint.y;
	}

	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		Polygon& poly = finishedPolygons[shapeIndex];
		poly.setVertices(newVertices);
		poly.updateBuffers();
		break;
	}
	case SHAPE_BEZIER:
	{
		Bezier& bezier = finishedBeziers[shapeIndex];
		bezier.setControlPoints(newVertices);
		bezier.generateCurve();
		bezier.updateBuffers();
		break;
	}
	}
}

void PolyBuilder::tryFindingIntersections()
{
	if (finishedBeziers.size() < 2)
		return;

	for (int i = 0; i < finishedBeziers.size() - 1; i++)
	{
		std::vector<Vertex> hullA = finishedBeziers[i].getConvexHull();
		std::vector<Vertex> hullB = finishedBeziers[i + 1].getConvexHull();

		bool result = testHullIntersection(hullA, hullB);

		if (result)
		{
			std::vector<Vertex> foundIntersections = findBezierIntersections(finishedBeziers[i], finishedBeziers[i + 1], 0.005f, 10);
			if (!foundIntersections.empty())
			{
				for (const auto& intersection : foundIntersections)
					intersections.addPoint(intersection);

				foundIntersectionsText.push_back(u8"Intersection found on Bézier " + std::to_string(i) + " and " + std::to_string(i + 1));
			}
		}
	}
}

bool PolyBuilder::testHullIntersection(const std::vector<Vertex> shapeA, const std::vector<Vertex> shapeB)
{
	// Make list of all normal vectors
	// Those are our potential separating axes
	std::vector<Vertex> normals;
	normals.resize(shapeA.size() + shapeB.size());

	for (int i = 0; i < shapeA.size(); i++)
	{
		Vertex p1 = shapeA[i];
		Vertex p2 = shapeA[i + 1 == shapeA.size() ? 0 : i + 1]; // Loop back to first vertex
		Vertex edge = p1 - p2; // edge vector
		Vertex normal = { -edge.y, edge.x };
		normals.push_back(normal);
	}

	for (int i = 0; i < shapeB.size(); i++)
	{
		Vertex p1 = shapeB[i];
		Vertex p2 = shapeB[i + 1 == shapeB.size() ? 0 : i + 1]; // Loop back to first vertex
		Vertex edge = p1 - p2; // edge vector
		Vertex normal = { -edge.y, edge.x };
		normals.push_back(normal);
	}

	// Project each polygon onto each potential separating axes
	for (const Vertex& axis : normals)
	{
		float minProjA = 99.0f;
		float maxProjA = -99.0f;

		for (const Vertex& vertex : shapeA)
		{
			// TODO : maybe make a dot product function somewhere ?
			float projection = (vertex.x * axis.x) + (vertex.y * axis.y);

			if (projection < minProjA)
				minProjA = projection;
			if (projection > maxProjA)
				maxProjA = projection;
		}

		float minProjB = 99.0f;
		float maxProjB = -99.0f;

		for (const Vertex& vertex : shapeB)
		{
			// TODO : maybe make a dot product function somewhere ?
			float projection = (vertex.x * axis.x) + (vertex.y * axis.y);

			if (projection < minProjB)
				minProjB = projection;
			if (projection > maxProjB)
				maxProjB = projection;
		}

		// If there's no overlap, then we found a separating axis, so no intersection
		if (maxProjA < minProjB || maxProjB < minProjA)
			return false;
	}

	// If there's overlap on every axis, there's an intersection
	return true;
}

void PolyBuilder::appendVertex(double xPos, double yPos)
{
	if (!buildingShape)
		return;

	// Get window size
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

	// Convert to normalized device coordinates (-1 to 1)
	float normalizedX = (2.0f * xPos / width) - 1.0f;
	float normalizedY = 1.0f - (2.0f * yPos / height); // Flip Y coordinate

	std::cout << "Added a vertex at X : " << std::to_string(normalizedX) << ", Y : " << std::to_string(normalizedY) <<
		std::endl;

	if (bezierMode)
	{
		tempBezier.addControlPoint(normalizedX, normalizedY);
		if (tempBezier.getControlPoints().size() > 2)
			tempBezier.generateCurve();
		tempBezier.updateBuffers();
	}
	else if (cubicSequenceMode)
	{
		appendToCubicSequence(normalizedX, normalizedY);
	}
	else
	{
		tempPolygon.addVertex(normalizedX, normalizedY);
		tempPolygon.updateBuffers();
	}
}

void PolyBuilder::finish()
{
	if (bezierMode)
		finishBezier();
	else if (cubicSequenceMode)
		finishCubicSequence();
	else
		finishPolygon();
}

void PolyBuilder::finishPolygon()
{
	std::cout << "Finishing polygon ..." << std::endl;

	if (!buildingShape)
		return;

	if (tempPolygon.isClockwise())
	{
		std::cout << "Polygon is clockwise. Reversing it to counter-clockwise." << std::endl;
		tempPolygon.reverseOrientation();
	}


	switch (polyType)
	{
	case (POLYGON):
		polygon = tempPolygon;
		polygon.type = POLYGON;
		polygon.updateBuffers();
		finishedPolygons.push_back(polygon);
		break;

	case (WINDOW):
		window = tempPolygon;
		window.type = WINDOW;
		window.updateBuffers();
		finishedPolygons.push_back(window);
		break;
	}

	buildingShape = false;
	tempPolygon = Polygon();
}

void PolyBuilder::finishBezier()
{
	std::cout << "Finishing Bézier ..." << std::endl;

	if (!buildingShape)
		return;

	bezier = tempBezier;
	bezier.generateConvexHull();
	bezier.generateCurve();
	bezier.updateBuffers();
	finishedBeziers.push_back(bezier);
	buildingShape = false;
	toggleBezierMode();
	tempBezier = Bezier();
}

void PolyBuilder::startCubicSequence()
{
	cubicSequenceMode = true;
	currentSequence = CubicBezierSequence(continuityType);
	tempBezier = Bezier();
	buildingShape = true;
}

void PolyBuilder::appendToCubicSequence(float x, float y) {
	std::vector<Bezier>& curves = currentSequence.getCurves();

	if (tempBezier.getControlPoints().empty() && !curves.empty()) {
		// Starting a new curve in the sequence, use the last point from previous curve
		tempBezier.addControlPoint(curves.back().getControlPoints().back());
	}

	// Add the new point
	tempBezier.addControlPoint(x, y);
	if (tempBezier.getControlPoints().size() > 2)
		tempBezier.generateCurve();
	tempBezier.updateBuffers();

	// If we now have 4 points, we have a complete cubic curve
	if (tempBezier.getControlPoints().size() == 4) {
		tempBezier.generateCurve();
		tempBezier.updateBuffers();
		currentSequence.addCurve(tempBezier);
		currentSequence.enforceConstraints(); // Apply continuity constraints

		// Reset for the next curve
		tempBezier = Bezier();
	}
}

void PolyBuilder::finishCubicSequence() {
	// If we have a partially completed curve (less than 4 points), discard it
	if (tempBezier.getControlPoints().size() < 4) {
		tempBezier = Bezier();
	}

	// Add the completed sequence to our collection
	if (!currentSequence.getCurves().empty()) {
		if (currentSequence.shouldBeClosed())
		{
			currentSequence.makeClosed();
			std::cout << "New sequence is closed !" << std::endl;
		}
		currentSequence.calculateGenerationTime();
		finishedSequences.push_back(currentSequence);
	}

	// Reset state
	currentSequence = CubicBezierSequence(continuityType);
	cubicSequenceMode = false;
	buildingShape = false;
}

// For transformations that should take place on the center, like scaling or shearing
Vertex PolyBuilder::calculateCenter(const std::vector<Vertex>& vertices)
{
	if (vertices.empty()) return { 0.0f, 0.0f };
	float sumX = 0.0f;
	float sumY = 0.0f;
	for (const auto& vert : vertices)
	{
		sumX += vert.x;
		sumY += vert.y;
	}
	return { sumX / vertices.size(), sumY / vertices.size() };
}

void PolyBuilder::cancel()
{
	buildingShape = false;
	tempPolygon = Polygon();
	tempBezier = Bezier();
}

void PolyBuilder::deleteVertex(int shapeIndex, int vertexIndex, ShapeType shapeType)
{
	std::vector<Vertex> vertices;

	switch (shapeType)
	{
	case SHAPE_POLYGON:
	{
		if (shapeIndex < 0 || shapeIndex >= finishedPolygons.size())
		{
			std::cerr << "Error: Invalid polygon index " << shapeIndex << std::endl;
			return;
		}

		Polygon& poly = finishedPolygons[shapeIndex];
		vertices = poly.getVertices();

		// Check vertex index validity
		if (vertexIndex < 0 || vertexIndex >= vertices.size())
		{
			std::cerr << "Error: Invalid vertex index " << vertexIndex << " for polygon " << shapeIndex << std::endl;
			return;
		}

		vertices.erase(vertices.begin() + vertexIndex);

		if (vertices.size() < 3)
		{
			std::cout << "Polygon " << shapeIndex << " has less than 3 vertices after deletion. Removing polygon." << std::endl;
			finishedPolygons.erase(finishedPolygons.begin() + shapeIndex);
			return; // Polygon removed
		}

		poly.setVertices(vertices);
		poly.updateBuffers();
		break;
	}
	case SHAPE_BEZIER:
	{
		// Check index validity for the Bezier list
		if (shapeIndex < 0 || shapeIndex >= finishedBeziers.size())
		{
			std::cerr << "Error: Invalid Bezier index " << shapeIndex << std::endl;
			return;
		}
		Bezier& bezier = finishedBeziers[shapeIndex];
		vertices = bezier.getControlPoints();

		// Check vertex index validity
		if (vertexIndex < 0 || vertexIndex >= vertices.size())
		{
			std::cerr << "Error: Invalid control point index " << vertexIndex << " for Bezier " << shapeIndex << std::endl;
			return;
		}

		vertices.erase(vertices.begin() + vertexIndex);

		if (vertices.size() < 2)
		{
			std::cout << "Bezier " << shapeIndex << " has less than 2 control points after deletion. Removing Bezier." << std::endl;
			finishedBeziers.erase(finishedBeziers.begin() + shapeIndex);
			return; // Bezier removed
		}

		bezier.setControlPoints(vertices);
		bezier.generateCurve();
		bezier.updateBuffers();
		break;
	}
	case SHAPE_BEZIER_SEQUENCE:
	{
		// TODO : this case
		break;
	}
	}
}

// Add a filled polygon to our storage
void PolyBuilder::addFilledPolygon(const Polygon& poly,
	const std::vector<Vertex>& fillPoints,
	float r, float g, float b, float a)
{
	FilledPolygon filled;
	filled.polygon = poly;
	filled.fillPoints = fillPoints;
	filled.colorR = r;
	filled.colorG = g;
	filled.colorB = b;
	filled.colorA = a;

	// Create OpenGL buffers for the fill points
	glGenVertexArrays(1, &filled.vao);
	glGenBuffers(1, &filled.vbo);

	// Upload fill points to GPU
	glBindVertexArray(filled.vao);
	glBindBuffer(GL_ARRAY_BUFFER, filled.vbo);
	glBufferData(GL_ARRAY_BUFFER,
		fillPoints.size() * sizeof(Vertex),
		fillPoints.data(),
		GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	// Add to our collection
	filledPolygons.push_back(filled);
}

// Clear all filled polygons
void PolyBuilder::clearFilledPolygons()
{
	for (auto& filled : filledPolygons)
	{
		// Delete OpenGL resources
		if (filled.vao != 0)
		{
			glDeleteVertexArrays(1, &filled.vao);
			glDeleteBuffers(1, &filled.vbo);
		}
	}
	filledPolygons.clear();
}

const std::vector<Polygon>& PolyBuilder::getFinishedPolygons() const
{
	return finishedPolygons;
}

void PolyBuilder::setFinishedPolygons(std::vector<Polygon> newFinishedPolygons)
{
	finishedPolygons = newFinishedPolygons;
}

const std::vector<FilledPolygon>& PolyBuilder::getFilledPolygons() const
{
	return filledPolygons;
}

void PolyBuilder::addFinishedPolygon(const Polygon& polygon)
{
	finishedPolygons.push_back(polygon);
}

void PolyBuilder::removeFinishedPolygon(int index)
{
	if (index >= 0 && index < finishedPolygons.size())
		finishedPolygons.erase(finishedPolygons.begin() + index);
}

void PolyBuilder::removeAllPolygonsOfType(PolyType type)
{
	auto it = finishedPolygons.begin();
	while (it != finishedPolygons.end()) {
		if (it->type == type)
			it = finishedPolygons.erase(it);
		else
			++it;
	}
}

Polygon& PolyBuilder::getPolygonAt(size_t index)
{
	if (index >= finishedPolygons.size())
		throw std::out_of_range("Polygon index out of range");

	return finishedPolygons[index];
}

bool PolyBuilder::isValidPolygonIndex(int index) const
{
	return index >= 0 && index < static_cast<int>(finishedPolygons.size());
}

bool PolyBuilder::isBuilding() const
{
	return buildingShape;
}

std::vector<Vertex> PolyBuilder::findBezierIntersections(const Bezier& curve1, const Bezier& curve2,
	float flatnessThreshold, int maxDepth) {
	std::vector<Vertex> intersections;

	// Helper function for recursive subdivision
	std::function<void(const Bezier&, const Bezier&, int)> findIntersectionsRecursive =
		[&](const Bezier& c1, const Bezier& c2, int depth) {

		// Generate convex hulls for both curves
		Bezier c1Copy = c1;
		Bezier c2Copy = c2;
		c1Copy.generateConvexHull();
		c2Copy.generateConvexHull();

		// Check if convex hulls intersect
		// This would use your SAT implementation
		bool hullsIntersect = testHullIntersection(c1Copy.getConvexHull(), c2Copy.getConvexHull());

		if (!hullsIntersect) {
			// No intersection, early exit
			return;
		}

		// Calculate flatness of both curves
		// (You'll need to implement this function - see below)
		float flatness1 = c1.calculateFlatness();
		float flatness2 = c2.calculateFlatness();

		// Base case: Both curves are approximately flat or maximum depth reached
		if ((flatness1 < flatnessThreshold && flatness2 < flatnessThreshold) || depth >= maxDepth) {
			// Treat as line segments
			Vertex intersection;
			std::vector<Vertex> c1ControlPoints = c1.getControlPoints();
			std::vector<Vertex> c2ControlPoints = c2.getControlPoints();
			if (lineSegmentsIntersect(
				c1ControlPoints.front(), c1ControlPoints.back(),
				c2ControlPoints.front(), c2ControlPoints.back(),
				intersection)) {
				// Check if this intersection is already in our list (within some epsilon)
				bool isDuplicate = false;
				for (const auto& existing : intersections) {
					if (squaredDistance(existing, intersection) < 1e-6) {
						isDuplicate = true;
						break;
					}
				}

				if (!isDuplicate) {
					intersections.push_back(intersection);
				}
			}
			return;
		}

		// Recursive case: Subdivide curves
		auto [c1Left, c1Right] = c1.subdivide(0.5f);
		auto [c2Left, c2Right] = c2.subdivide(0.5f);

		// Test all four combinations
		findIntersectionsRecursive(c1Left, c2Left, depth + 1);
		findIntersectionsRecursive(c1Left, c2Right, depth + 1);
		findIntersectionsRecursive(c1Right, c2Left, depth + 1);
		findIntersectionsRecursive(c1Right, c2Right, depth + 1);
		};

	// Start the recursive process
	findIntersectionsRecursive(curve1, curve2, 0);

	return intersections;
}
