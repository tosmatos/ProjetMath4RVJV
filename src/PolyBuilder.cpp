#include "PolyBuilder.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <string>

namespace PolyBuilder
{
	Type polyType;
	bool buildingPoly;
	Polygon tempPolygon; // The one for the building process
	Polygon polygon;
	Polygon window;
	std::vector<Polygon> finishedPolygons;

	void StartPolygon(Type type)
	{
		polyType = type;
		buildingPoly = true;
		tempPolygon = Polygon();
	}

	void AppendVertex(double xPos, double yPos)
	{
		if (!buildingPoly)
			return;

		// Get window size
		int width, height;
		glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

		// Convert to normalized device coordinates (-1 to 1)
		float normalizedX = (2.0f * xPos / width) - 1.0f;
		float normalizedY = 1.0f - (2.0f * yPos / height);  // Flip Y coordinate

		std::cout << "Added a vertex at X : " << std::to_string(normalizedX) << ", Y : " << std::to_string(normalizedY);

		tempPolygon.addVertex(normalizedX, normalizedY);
	}

	void Finish()
	{
		std::cout << "Finishing polygon ..." << std::endl;		

		if (!buildingPoly)
			return;

		std::cout << "Polygon is " << (tempPolygon.isClockwise() ? "clockwise" : "counter-clockwise.") << std::endl;
		// if not clockwise, reverse the orientation
		// WARNING : Maybe not the best thing to do, don't really know.
		if (!tempPolygon.isClockwise())
		{
			tempPolygon.reverseOrientation();
			std::cout << "Reverse polygon orientation to clockwise." << std::endl;
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

		

		buildingPoly = false;
		tempPolygon = Polygon();
	}

	void Cancel()
	{
		buildingPoly = false;
		tempPolygon = Polygon();
	}
}