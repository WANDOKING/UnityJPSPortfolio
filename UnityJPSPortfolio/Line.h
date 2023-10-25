// 시작점부터 끝점까지의 직선을 구성하는 좌표들을 얻어낼 수 있습니다.
// 브레젠험 직선 알고리듬을 기반으로 작성되었습니다.

/************************************** 사용법 **************************************/
// Line line(startX, startY, endX, endY);
// 
// for (auto it = line.Begin(); it != line.End(); ++it)
// {
//     ...
// }
/************************************************************************************/

#pragma once

#include <list>
#include "Point.h"

class Line
{
public:
	Line(int startX, int startY, int endX, int endY)
	{
		int deltaX = (startX - endX) > 0 ? (startX - endX) : -1 * (startX - endX);
		int deltaY = (startY - endY) > 0 ? (startY - endY) : -1 * (startY - endY);

		int relativeX = endX > startX ? 1 : -1;
		int relativeY = endY > startY ? 1 : -1;

		mPoints.push_back(Point{ startX, startY });

		if (deltaX >= deltaY)
		{
			int yCarry = 0;
			int ySum = 0;

			for (int i = 1; i < deltaX; ++i)
			{
				ySum += deltaY + 1;

				if (ySum >= deltaX + 1)
				{
					ySum -= deltaX + 1;
					yCarry++;
				}

				mPoints.push_back(Point{ startX + relativeX * i, startY + relativeY * yCarry });
			}
		}
		else
		{
			int xCarry = 0;
			int xSum = 0;

			for (int i = 1; i < deltaY; ++i)
			{
				xSum += deltaX + 1;

				if (xSum >= deltaY + 1)
				{
					xSum -= deltaY + 1;
					xCarry++;
				}

				mPoints.push_back(Point{ startX + relativeX * xCarry, startY + relativeY * i });
			}
		}

		mPoints.push_back(Point{ endX, endY });
	}

	inline Point GetStart()
	{
		return mPoints.front();
	}

	inline Point GetEnd()
	{
		return mPoints.back();
	}

	const std::list<Point>::iterator Begin()
	{
		return mPoints.begin();
	}

	const std::list<Point>::iterator End()
	{
		return mPoints.end();
	}
private:
	std::list<Point> mPoints;
};