#pragma once

struct Vertex
{
	float x, y;

	Vertex operator+(Vertex other) const
	{
		return Vertex{ x + other.x, y + other.y };
	}

	Vertex operator-(Vertex other) const
	{
		return Vertex{ x - other.x, y - other.y };
	}

	Vertex operator*(float b)
	{
		return Vertex{ x * b, y * b };
	}

	Vertex& operator+=(Vertex other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	Vertex(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
};
