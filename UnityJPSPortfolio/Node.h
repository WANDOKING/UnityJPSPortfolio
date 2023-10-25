#pragma once

#include <cmath>

struct Node
{
public:
	int F;
	int G;
	int H;
	int X;
	int Y;
	Node* Parent;

public:
	Node(int x, int y, int g, Node* parent, int destinationX, int destinationY)
		: X(x)
		, Y(y)
		, G(g)
		, Parent(parent)
	{
		H = getHByRealMinDistance(destinationX, destinationY);

		F = G + H;
	}

private:
	// 맨하튼 거리
	int getHByManhattan(int destinationX, int destinationY)
	{
		int xGap = abs(X - destinationX);
		int yGap = abs(Y - destinationY);
		return (xGap + yGap) * 5;
	}

	// 실제 거리 비용
	int getHByRealMinDistance(int destinationX, int destinationY)
	{
		int xGap = abs(X - destinationX);
		int yGap = abs(Y - destinationY);
		int minGap = xGap < yGap ? xGap : yGap;
		int maxGap = xGap > yGap ? xGap : yGap;
		return minGap * 7 + (maxGap - minGap) * 5;
	}
public:
	bool operator>(const Node& other)
	{
		if (F == other.F)
		{
			return H < other.H;
		}

		return F < other.F;
	}

	bool operator>=(const Node& other)
	{
		if (F == other.F)
		{
			return H <= other.H;
		}

		return F <= other.F;
	}

	bool operator<(const Node& other)
	{
		if (F == other.F)
		{
			return H > other.H;
		}

		return F > other.F;
	}

	bool operator<=(const Node& other)
	{
		if (F == other.F)
		{
			return H >= other.H;
		}

		return F >= other.F;
	}

	bool operator==(const Node& other)
	{
		return F == other.F && H == other.H;
	}
};