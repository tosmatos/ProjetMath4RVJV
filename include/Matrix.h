#pragma once
#include <cmath> // For cos and sin

#include "Vertex.h"

/* Having a 3x3 matrix for 2D transformations is because we want to use
* homogeneous coordinates. This lets us combine different transformations with ease
* and is a standard in computer graphics it seems.
* I'll admit this hasn't clicked for me yet. It will. */

struct Matrix3x3
{
	float elements[3][3];

	Matrix3x3()
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				elements[i][j] = (i == j) ? 1.0f : 0.0f;
			}
		}
	}

	Matrix3x3 operator*(const Matrix3x3 other) const
	{
		Matrix3x3 result;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				for (int k = 0; k < 3; k++)
				{
					result.elements[i][j] += elements[i][k] * other.elements[k][j];
				}
			}
		}

		return result;
	}
};

inline Matrix3x3 createTranslationMatrix(float tx, float ty)
{
	Matrix3x3 outMatrix;

	outMatrix.elements[0][2] = tx; // first row, third column
	outMatrix.elements[1][2] = ty; // second row, third column

	return outMatrix;
}

// Angle should be in radians I think
inline Matrix3x3 createRotationMatrix(float angle)
{
	Matrix3x3 outMatrix;

	outMatrix.elements[0][0] = cos(angle); // first row first column
	outMatrix.elements[0][1] = -sin(angle); // first row second column
	outMatrix.elements[1][0] = sin(angle); // second row first column
	outMatrix.elements[1][1] = cos(angle); // second row second column

	return outMatrix;
}

inline Matrix3x3 createScalingMatrix(float sx, float sy)
{
	Matrix3x3 outMatrix;

	outMatrix.elements[0][0] = sx; // first row first column
	outMatrix.elements[1][1] = sy; // second row second column

	return outMatrix;
}

inline Matrix3x3 createShearingMatrix(float shx, float shy)
{
	Matrix3x3 outMatrix;

	outMatrix.elements[0][1] = shx; // first row second column
	outMatrix.elements[1][0] = shy; // second row first column

	return outMatrix;
}

// With implicit w = 1
inline Vertex multiplyMatrixVertex(const Matrix3x3 matrix, const Vertex& inVertex)
{
	Vertex outVertex;

	outVertex.x = matrix.elements[0][0] * inVertex.x + matrix.elements[0][1] * inVertex.y + matrix.elements[0][2] * 1.0f;
	outVertex.y = matrix.elements[1][0] * inVertex.x + matrix.elements[1][1] * inVertex.y + matrix.elements[1][2] * 1.0f;

	return outVertex;
}