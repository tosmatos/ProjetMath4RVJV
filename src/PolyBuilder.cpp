#include "Polygon.h"

namespace PolyBuilder
{
	enum Type { POLYGON, WINDOW, };
	Type polyType;
	bool buildingPoly;
	Polygon tempPolygon; // The one for the building process
	Polygon polygon;
	Polygon window;

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

		// TODO : Process screen coordinate to normalized coordinates

		tempPolygon.addVertex(xPos, yPos);
	}

	void Finish()
	{
		if (!buildingPoly)
			return;

		switch (polyType)
		{
			case (POLYGON):
				polygon = tempPolygon;
				break;

			case (WINDOW):
				window = tempPolygon;
				break;
		}

		buildingPoly = false;
		tempPolygon.~Polygon();
	}
}