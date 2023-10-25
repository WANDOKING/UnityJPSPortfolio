#pragma once

#include "PriorityQueue.h"
#include <list>
#include "Line.h"
#include "Point.h"

class JPSPathFinder
{
public:
	JPSPathFinder(int mapWidth, int mapHeight)
		: mWidth(mapWidth)
		, mHeight(mapHeight)
		, mOpenList(mapWidth* mapHeight)
	{
		mMap = new bool* [mapHeight];
		mbOpenListMade = new bool* [mapHeight];
		mG = new int* [mapHeight];

		for (int i = 0; i < mapHeight; ++i)
		{
			mMap[i] = new bool[mapWidth];
			mbOpenListMade[i] = new bool[mapWidth];
			mG[i] = new int[mapWidth];

			for (int j = 0; j < mapWidth; ++j)
			{
				mMap[i][j] = true;
				mbOpenListMade[i][j] = true;
				mG[i][j] = -1;
			}
		}
	}

	~JPSPathFinder()
	{
		for (int i = 0; i < mHeight; ++i)
		{
			delete[] mMap[i];
			delete[] mG[i];
			delete[] mbOpenListMade[i];
		}

		delete mMap;
		delete mG;
		delete mbOpenListMade;
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

	std::list<Point>::iterator PathFind(int startX, int startY, int endX, int endY)
	{
		//PROFILE(L"JPS");

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
			if (currentNode->Parent == nullptr)
			{
				// 시작 노드인 경우
				PathCheckLL(currentNode, endX, endY);
				PathCheckRR(currentNode, endX, endY);
				PathCheckUU(currentNode, endX, endY);
				PathCheckDD(currentNode, endX, endY);
				PathCheckLU(currentNode, endX, endY);
				PathCheckLD(currentNode, endX, endY);
				PathCheckRU(currentNode, endX, endY);
				PathCheckRD(currentNode, endX, endY);
			}
			else if (currentNode->Parent->Y == currentNode->Y && currentNode->Parent->X < currentNode->X)
			{
				//
				// [P] -> (N)
				//
				PathCheckRR(currentNode, endX, endY);
				PathCheckRU(currentNode, endX, endY);
				PathCheckRD(currentNode, endX, endY);
			}
			else if (currentNode->Parent->Y == currentNode->Y && currentNode->Parent->X > currentNode->X)
			{
				//
				// (N) <- [P]
				//
				PathCheckLL(currentNode, endX, endY);
				PathCheckLD(currentNode, endX, endY);
				PathCheckLU(currentNode, endX, endY);
			}
			else if (currentNode->Parent->X == currentNode->X && currentNode->Parent->Y > currentNode->Y)
			{
				//    (N)
				//     ↑
				//    [P]
				PathCheckUU(currentNode, endX, endY);
				PathCheckLU(currentNode, endX, endY);
				PathCheckRU(currentNode, endX, endY);
			}
			else if (currentNode->Parent->X == currentNode->X && currentNode->Parent->Y < currentNode->Y)
			{
				//    [P]
				//     ↓
				//    (N)
				PathCheckDD(currentNode, endX, endY);
				PathCheckLD(currentNode, endX, endY);
				PathCheckRD(currentNode, endX, endY);
			}
			else if (currentNode->Parent->X < currentNode->X && currentNode->Parent->Y < currentNode->Y)
			{
				// [P]
				//    ↘ 
				//      (N)

				if (IsBlocked(currentNode->X, currentNode->Y - 1) == true && IsBlocked(currentNode->X + 1, currentNode->Y - 1) == false)
				{
					PathCheckRU(currentNode, endX, endY);
				}

				PathCheckRR(currentNode, endX, endY);
				PathCheckRD(currentNode, endX, endY);
				PathCheckDD(currentNode, endX, endY);

				if (IsBlocked(currentNode->X - 1, currentNode->Y) == true && IsBlocked(currentNode->X - 1, currentNode->Y + 1) == false)
				{
					PathCheckLD(currentNode, endX, endY);
				}
			}
			else if (currentNode->Parent->X > currentNode->X && currentNode->Parent->Y > currentNode->Y)
			{
				// (N)
				//    ↖ 
				//      [P]

				if (IsBlocked(currentNode->X, currentNode->Y + 1) == true && IsBlocked(currentNode->X - 1, currentNode->Y + 1) == false)
				{
					PathCheckLD(currentNode, endX, endY);
				}

				PathCheckLL(currentNode, endX, endY);
				PathCheckLU(currentNode, endX, endY);
				PathCheckUU(currentNode, endX, endY);

				if (IsBlocked(currentNode->X + 1, currentNode->Y) == true && IsBlocked(currentNode->X + 1, currentNode->Y - 1) == false)
				{
					PathCheckRU(currentNode, endX, endY);
				}
			}
			else if (currentNode->Parent->X < currentNode->X && currentNode->Parent->Y > currentNode->Y)
			{
				//       (N)
				//     ↗
				// [P]
				if (IsBlocked(currentNode->X - 1, currentNode->Y) == true && IsBlocked(currentNode->X - 1, currentNode->Y - 1) == false)
				{
					PathCheckLU(currentNode, endX, endY);
				}

				PathCheckUU(currentNode, endX, endY);
				PathCheckRU(currentNode, endX, endY);
				PathCheckRR(currentNode, endX, endY);

				if (IsBlocked(currentNode->X, currentNode->Y + 1) == true && IsBlocked(currentNode->X + 1, currentNode->Y + 1) == false)
				{
					PathCheckRD(currentNode, endX, endY);
				}
			}
			else if (currentNode->Parent->X > currentNode->X && currentNode->Parent->Y < currentNode->Y)
			{
				//       [P]
				//     ↙
				// (N)
				if (IsBlocked(currentNode->X + 1, currentNode->Y) == true && IsBlocked(currentNode->X + 1, currentNode->Y + 1) == false)
				{
					PathCheckRD(currentNode, endX, endY);
				}

				PathCheckDD(currentNode, endX, endY);
				PathCheckLD(currentNode, endX, endY);
				PathCheckLL(currentNode, endX, endY);

				if (IsBlocked(currentNode->X, currentNode->Y - 1) == true && IsBlocked(currentNode->X - 1, currentNode->Y - 1) == false)
				{
					PathCheckLU(currentNode, endX, endY);
				}
			}
			else
			{
				assert(false);
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
				mbOpenListMade[i][j] = false;
			}
		}
	}

private:
	//Path Check

	void PathCheckLL(Node* node, int endX, int endY)
	{
		int x = node->X - 1;
		int y = node->Y;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathUp = IsBlocked(x, y - 1) == true && IsBlocked(x - 1, y - 1) == false;
			bool bNewPathDown = IsBlocked(x, y + 1) == true && IsBlocked(x - 1, y + 1) == false;

			if (bNewPathUp || bNewPathDown)
			{
				bCreateNode = true;
				break;
			}

			x--;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + (node->X - x) * 5, node, endX, endY);
		}
	}

	void PathCheckRR(Node* node, int endX, int endY)
	{
		int x = node->X + 1;
		int y = node->Y;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathUp = IsBlocked(x, y - 1) == true && IsBlocked(x + 1, y - 1) == false;
			bool bNewPathDown = IsBlocked(x, y + 1) == true && IsBlocked(x + 1, y + 1) == false;

			if (bNewPathUp || bNewPathDown)
			{
				bCreateNode = true;
				break;
			}

			x++;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + (x - node->X) * 5, node, endX, endY);
		}
	}

	void PathCheckUU(Node* node, int endX, int endY)
	{
		int x = node->X;
		int y = node->Y - 1;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathUp = IsBlocked(x - 1, y) == true && IsBlocked(x - 1, y - 1) == false;
			bool bNewPathDown = IsBlocked(x + 1, y) == true && IsBlocked(x + 1, y - 1) == false;

			if (bNewPathUp || bNewPathDown)
			{
				bCreateNode = true;
				break;
			}

			y--;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + (node->Y - y) * 5, node, endX, endY);
		}
	}

	void PathCheckDD(Node* node, int endX, int endY)
	{
		int x = node->X;
		int y = node->Y + 1;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathUp = IsBlocked(x - 1, y) == true && IsBlocked(x - 1, y + 1) == false;
			bool bNewPathDown = IsBlocked(x + 1, y) == true && IsBlocked(x + 1, y + 1) == false;

			if (bNewPathUp || bNewPathDown)
			{
				bCreateNode = true;
				break;
			}

			y++;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + (y - node->Y) * 5, node, endX, endY);
		}
	}

	void PathCheckLU(Node* node, int endX, int endY)
	{
		int x = node->X - 1;
		int y = node->Y - 1;
		int i = 0;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathSide1 = IsBlocked(x + 1, y) == true && IsBlocked(x + 1, y - 1) == false;
			bool bNewPathSide2 = IsBlocked(x, y + 1) == true && IsBlocked(x - 1, y + 1) == false;

			if (bNewPathSide1 || bNewPathSide2)
			{
				bCreateNode = true;
				break;
			}

			// ←
			i = 1;
			while (true)
			{
				if (x - i == endX && y == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x - i, y))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - i, y - 1) == true && IsBlocked(x - i - 1, y - 1) == false;
				bool bNewPathDown = IsBlocked(x - i, y + 1) == true && IsBlocked(x - i - 1, y + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			// ↑
			i = 1;
			while (true)
			{
				if (x == endX && y - i == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x, y - i))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - 1, y - i) == true && IsBlocked(x - 1, y - i - 1) == false;
				bool bNewPathDown = IsBlocked(x + 1, y - i) == true && IsBlocked(x + 1, y - i - 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			x--;
			y--;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + abs(node->Y - y) * 7, node, endX, endY);
		}
	}

	void PathCheckLD(Node* node, int endX, int endY)
	{
		int x = node->X - 1;
		int y = node->Y + 1;
		int i = 0;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathSide1 = IsBlocked(x, y - 1) == true && IsBlocked(x - 1, y - 1) == false;
			bool bNewPathSide2 = IsBlocked(x + 1, y) == true && IsBlocked(x + 1, y + 1) == false;

			if (bNewPathSide1 || bNewPathSide2)
			{
				bCreateNode = true;
				break;
			}

			// ←
			i = 1;
			while (true)
			{
				if (x - i == endX && y == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x - i, y))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - i, y - 1) == true && IsBlocked(x - i - 1, y - 1) == false;
				bool bNewPathDown = IsBlocked(x - i, y + 1) == true && IsBlocked(x - i - 1, y + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			// ↓
			i = 1;
			while (true)
			{
				if (x == endX && y + i == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x, y + i))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - 1, y + i) == true && IsBlocked(x - 1, y + i + 1) == false;
				bool bNewPathDown = IsBlocked(x + 1, y + i) == true && IsBlocked(x + 1, y + i + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			x--;
			y++;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + abs(y - node->Y) * 7, node, endX, endY);
		}
	}

	void PathCheckRU(Node* node, int endX, int endY)
	{
		int x = node->X + 1;
		int y = node->Y - 1;
		int i = 0;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathSide1 = IsBlocked(x - 1, y) == true && IsBlocked(x - 1, y - 1) == false;
			bool bNewPathSide2 = IsBlocked(x, y + 1) == true && IsBlocked(x + 1, y + 1) == false;

			if (bNewPathSide1 || bNewPathSide2)
			{
				bCreateNode = true;
				break;
			}

			// →
			i = 1;
			while (true)
			{
				if (x + i == endX && y == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x + i, y))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x + i, y - 1) == true && IsBlocked(x + i + 1, y - 1) == false;
				bool bNewPathDown = IsBlocked(x + i, y + 1) == true && IsBlocked(x + i + 1, y + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			// ↑
			i = 1;
			while (true)
			{
				if (x == endX && y - i == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x, y - i))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - 1, y - i) == true && IsBlocked(x - 1, y - i - 1) == false;
				bool bNewPathDown = IsBlocked(x + 1, y - i) == true && IsBlocked(x + 1, y - i - 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			x++;
			y--;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + abs(node->Y - y) * 7, node, endX, endY);
		}
	}

	void PathCheckRD(Node* node, int endX, int endY)
	{
		int x = node->X + 1;
		int y = node->Y + 1;
		int i = 0;

		bool bCreateNode = false;

		while (true)
		{
			if (x == endX && y == endY)
			{
				bCreateNode = true;
				break;
			}

			if (IsBlocked(x, y))
			{
				break;
			}

			bool bNewPathSide1 = IsBlocked(x, y - 1) == true && IsBlocked(x + 1, y - 1) == false;
			bool bNewPathSide2 = IsBlocked(x - 1, y) == true && IsBlocked(x - 1, y + 1) == false;

			if (bNewPathSide1 || bNewPathSide2)
			{
				bCreateNode = true;
				break;
			}

			// →
			i = 1;
			while (true)
			{
				if (x + i == endX && y == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x + i, y))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x + i, y - 1) == true && IsBlocked(x + i + 1, y - 1) == false;
				bool bNewPathDown = IsBlocked(x + i, y + 1) == true && IsBlocked(x + i + 1, y + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			// ↓
			i = 1;
			while (true)
			{
				if (x == endX && y + i == endY)
				{
					bCreateNode = true;
					break;
				}

				if (IsBlocked(x, y + i))
				{
					break;
				}

				bool bNewPathUp = IsBlocked(x - 1, y + i) == true && IsBlocked(x - 1, y + i + 1) == false;
				bool bNewPathDown = IsBlocked(x + 1, y + i) == true && IsBlocked(x + 1, y + i + 1) == false;

				if (bNewPathUp || bNewPathDown)
				{
					bCreateNode = true;
					break;
				}

				i++;
			}

			if (bCreateNode)
			{
				break;
			}

			x++;
			y++;
		}

		if (bCreateNode)
		{
			CreateNode(x, y, node->G + abs(node->Y - y) * 7, node, endX, endY);
		}
	}

	// 노드 생성 & OPEN LIST에 push
	void CreateNode(int x, int y, int g, Node* parent, int endX, int endY)
	{
		if (mbOpenListMade[y][x] == true)
		{
			if (g < mG[y][x])
			{
				int outIndex;
				Node* node = mOpenList.GetNodeOrNull(x, y, outIndex);
				assert(node != nullptr);

				node->G = g;
				node->F = node->G + node->H;
				node->Parent = parent;
				mG[y][x] = node->G;

				mOpenList.RepairHeap(outIndex);
			}

			return;
		}

		Node* newNode = new Node(x, y, g, parent, endX, endY);
		mOpenList.Push(newNode);
		mbOpenListMade[y][x] = true;
		mG[y][x] = newNode->G;
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
	bool** mbOpenListMade;
	bool** mMap;
	int** mG;
};