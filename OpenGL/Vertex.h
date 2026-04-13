#pragma once


class Vertex
{
	private:
		float x, y, z; // Position
		float r, g, b; // Color
	public:
		Vertex(float x, float y, float z, float r, float g, float b);
		float getX() const;
		float getY() const;
		float getZ() const;
		float getR() const;
		float getG() const;
		float getB() const;
		void setX(float x);
		void setY(float y);
		void setZ(float z);
		void setR(float r);
		void setG(float g);
		void setB(float b);
};