#include "Point.h"
#include "NetLibrary/NetServer/Serializer.h"

Serializer& operator<<(Serializer& serializer, const Point& point)
{
	return serializer << point.X << point.Y;
}

Serializer& operator>>(Serializer& serializer, Point& point)
{
	return serializer >> point.Y >> point.X;
}