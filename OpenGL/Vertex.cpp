#include "Vertex.h"


Vertex::Vertex(float x, float y, float z, float r, float g, float b)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->r = r;
	this->g = g;
	this->b = b;
}
float Vertex::getX() const { return x; }
float Vertex::getY() const { return y; }
float Vertex::getZ() const { return z; }
float Vertex::getR() const { return r; }
float Vertex::getG() const { return g; }
float Vertex::getB() const { return b; }
void Vertex::setX(float x) { this->x = x; }
void Vertex::setY(float y) { this->y = y; }
void Vertex::setZ(float z) { this->z = z; }
void Vertex::setR(float r) { this->r = r; }
void Vertex::setG(float g) { this->g = g; }
void Vertex::setB(float b) { this->b = b; }


