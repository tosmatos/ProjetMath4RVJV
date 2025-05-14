#pragma once

#include <vector>
#include "Vertex.h"
#include "PolyTypes.h"
#include "Shader.h"

class Bezier
{
private:
	std::vector<Vertex> controlPoints; // Control points, what the user defined
	std::vector<Vertex> generatedCurve; // That's the actual curve when the user's finished
	std::vector<Vertex> convexHull;

	unsigned int controlVAO, curveVAO, hullVAO; // Vertex Array Object for saving VBO settings
	unsigned int controlVBO, curveVBO, hullVBO; // Vertex Buffer Object for each drawn shape

	bool buffersInitialized = false;

	float stepSize = 0.01f;
	int algorithm = 0; // 0 = normal pascal, 1 = De Casteljau (iterative)
	
	double generationTime = 0.0f;

	void generatePascalCurve();
	void generateDeCasteljauCurve();

public:
	//PolyType type; // Not sure this will be useful for now

	Bezier();
	Bezier(const Bezier& other); // Copy constructor
	Bezier& operator=(const Bezier& other); // Assignment operator
	~Bezier();

	void addControlPoint(float x, float y);
	void addControlPoint(Vertex vertex);

	void updateBuffers();
	const void drawControlPoints(Shader& shader) const;	
	const void drawGeneratedCurve(Shader& shader) const;
	const void drawControlPointsPreview(Shader& shader) const;
	const void drawGeneratedCurvePreview(Shader& shader) const;
	const void drawConvexHull(Shader& shader) const;

	const std::vector<Vertex>& getControlPoints() const { return controlPoints; };
	const std::vector<Vertex>& getGeneratedCurve() const { return generatedCurve; };
	void setControlPoints(std::vector<Vertex> controlPointsVector) { controlPoints = controlPointsVector; };
	void setConvexHull(std::vector<Vertex> newConvexHull) { convexHull = newConvexHull; };

	void generateCurve(); // When control points are set, this will be called
	const int getAlgorithm() const { return algorithm; };
	void swapAlgorithm() { algorithm = algorithm == 0 ? 1 : 0; generateCurve(); };

	const float getStepSize() const { return stepSize; };
	void setStepSize(float step) { stepSize = step; };
	void incrementStepSize();
	void decrementStepSize();

	const double getGenerationTime() const { return generationTime; };

	void duplicateControlPoint(int index);

	// Not sure if those two are gonna be necessary or useful
	//bool isClockwise() const;
	//void reverseOrientation(); // Makes polygon clockwise if counter clockwise and the opposite
};