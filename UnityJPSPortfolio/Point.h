#pragma once

struct Point
{
	int X;
	int Y;
};

class Serializer;

Serializer& operator<<(Serializer& serializer, const Point& point);
Serializer& operator>>(Serializer& serializer, Point& point);