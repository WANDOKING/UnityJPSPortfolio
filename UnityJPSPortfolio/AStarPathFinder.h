#pragma once

#include "PriorityQueue.h"
#include <list>
#include "Line.h"
#include "Point.h"

class AStarPathFinder
{
public:
	AStarPathFinder(int mapWidth, int mapHeight)
		: mWidth(mapWidth)
		, mHeight(mapHeight)
		, mOpenList(mapWidth* mapHeight)
	{
		mMap = new bool* [mapHeight];
		mG = new int* [mapHeight];

		for (int i = 0; i < mapHeight; ++i)
		{
			mMap[i] = new bool[mapWidth];
			mG[i] = new int[mapWidth];

			for (int j = 0; j < mapWidth; ++j)
			{
				mMap[i][j] = true;
				mG[i][j] = -1;
			}
		}
	}

	~AStarPathFinder()
	{
		for (int i = 0; i < mHeight; ++i)
		{
			delete[] mMap[i];
			delete[] mG[i];
		}

		delete mMap;
		delete mG;
	}

	inline std::list<Point> GetPoints() { return mPoints; }
	inline void Block(int x, int y) { mMap[y][x] = false; }
	inline void UnBlock(int x, int y) { mMap[y][x] = true; }
	inline bool IsBlocked(int x, int y)
	{
		// out of range
		if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
		{
			return true;
		}

		return !mMap[y][x];
	}

	inline const std::list<Point>::iterator Begin() { return mPoints.begin(); }
	inline const std::list<Point>::iterator End() { return mPoints.end(); }

	const std::list<Point>::iterator PathFind(int startX, int startY, int endX, int endY)
	{
		//PROFILE(L"AStar");

		if (startX < 0 || endX < 0 || startX >= mWidth || startY >= mHeight)
		{
			return End();
		}

		if (mMap[startY][startX] == false || mMap[endY][endX] == false)
		{
			return End();
		}

		Clear();

		Node* startNode = new Node(startX, startY, 0, nullptr, endX, endY);
		mOpenList.Push(startNode);

		while (mOpenList.Empty() == false)
		{
			Node* currentNode = mOpenList.Top();
			mOpenList.Pop();

			// Find
			if (currentNode->X == endX && currentNode->Y == endY)
			{
				reduceNodes(currentNode);

				Node* visit = currentNode;
				while (visit != nullptr)
				{
					mPoints.push_front(Point{ visit->X, visit->Y });
					visit = visit->Parent;
				}

				break;
			}

			// 8방향 검사
			// 8방향에 대한 상대 좌표 값
			const int X_RELATIVE[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };
			const int Y_RELATIVE[8] = { 0, -1, -1, -1,  0,  1,  1,  1 };
			const int G_RELATIVE[8] = { 5,  7,  5,  7,  5,  7,  5,  7 };

			for (int i = 0; i < 8; ++i)
			{
				int x = currentNode->X + X_RELATIVE[i];
				int y = currentNode->Y + Y_RELATIVE[i];
				int g = currentNode->G + G_RELATIVE[i];

				if (x < 0 || x >= mWidth || y < 0 || y >= mHeight)
				{
					continue;
				}

				if (mMap[y][x] == false)
				{
					continue;
				}

				if (mG[y][x] != -1 && g >= mG[y][x])
				{
					continue;
				}

				if (mG[y][x] != -1 && g < mG[y][x])
				{
					int outIndex;
					Node* node = mOpenList.GetNodeOrNull(x, y, outIndex);
					assert(node != nullptr);

					node->G = g;
					node->F = node->G + node->H;
					mG[y][x] = node->G;
					node->Parent = currentNode;

					mOpenList.RepairHeap(outIndex);
				}
				else
				{
					Node* newNode = new Node(x, y, g, currentNode, endX, endY);
					mG[y][x] = newNode->G;

					mOpenList.Push(newNode);
				}
			}
		}

		return Begin();
	}

	void Clear()
	{
		mPoints.clear();
		mOpenList.Clear();

		for (int i = 0; i < mHeight; ++i)
		{
			for (int j = 0; j < mWidth; ++j)
			{
				mG[i][j] = -1;
			}
		}
	}

private:
	// 불필요한 중간 노드들의 연결을 끊는다
	void reduceNodes(Node* destination) const
	{
		Node* startNode = destination;
		Node* endNode = destination->Parent;

		while (endNode != nullptr)
		{
			if (canIgnore(startNode, endNode))
			{
				startNode->Parent = endNode;
				endNode = endNode->Parent;
			}
			else
			{
				startNode = startNode->Parent;
			}
		}
	}

	// start ~ end 사이에 벽에 걸리는 것이 없는가
	bool canIgnore(const Node* start, const Node* end) const
	{
		Line line(start->X, start->Y, end->X, end->Y);

		for (auto it = line.Begin(); it != line.End(); ++it)
		{
			if (mMap[(*it).Y][(*it).X] == false)
			{
				return false;
			}
		}

		return true;
	}
private:
	std::list<Point> mPoints;
	PriorityQueue mOpenList;

	const int mWidth;
	const int mHeight;
	bool** mMap;
	int** mG;
};