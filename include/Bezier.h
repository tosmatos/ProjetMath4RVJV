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

	unsigned int controlVAO, curveVAO; // Vertex Array Object for saving VBO settings
	unsigned int controlVBO, curveVBO; // Vertex Buffer Object for each drawn shape

	bool buffersInitialized = false;

	int stepSize = 50;
	int algorithm = 0; // 0 = normal pascal, 1 = De Casteljau (iterative)

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

	const std::vector<Vertex>& getControlPoints() const { return controlPoints; };
	const std::vector<Vertex>& getGeneratedCurve() const { return generatedCurve; };
	void setControlPoints(std::vector<Vertex> controlPointsVector) { controlPoints = controlPointsVector; };

	void generateCurve(); // When control points are set, this will be called
	void setAlgorithm(int algo);
	void setStepSize(int step) { stepSize = step; };
	void incrementStepSize() { stepSize += 1; };
	void decrementStepSize() { stepSize -= 1; };

	// Not sure if those two are gonna be necessary or useful
	//bool isClockwise() const;
	//void reverseOrientation(); // Makes polygon clockwise if counter clockwise and the opposite
};