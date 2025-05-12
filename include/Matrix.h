#pragma once
#include "Vertex.h"

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
};

Matrix3x3 createTranslationMatrix(float tx, float ty)
{
	Matrix3x3 outMatrix;

	outMatrix.elements[0][2] = tx; // first row, third column
	outMatrix.elements[1][2] = ty; // second row, third column

	return outMatrix;
}

// With implicit w = 1
Vertex multiplyMatrixVertex(const Matrix3x3 matrix, const Vertex& inVertex)
{
	Vertex outVertex;

	outVertex.x = matrix.elements[0][0] * inVertex.x + matrix.elements[0][1] * inVertex.y + matrix.elements[0][2] * 1.0f;
	outVertex.y = matrix.elements[1][0] * inVertex.x + matrix.elements[1][1] * inVertex.y + matrix.elements[1][2] * 1.0f;

	return outVertex;
}