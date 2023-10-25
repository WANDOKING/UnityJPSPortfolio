#pragma once

#include <cassert>

#include "Node.h"

class PriorityQueue
{
public:
    PriorityQueue(int elementCount)
        : mSize(0)
    {
        mDatas = new Node * [elementCount];
        mCapacity = elementCount;
    }

    ~PriorityQueue()
    {
        delete[] mDatas;
    }

    void Push(Node* data)
    {
        if (mSize == mCapacity)
        {
            return;
        }

        mDatas[mSize] = data;

        RepairHeap(mSize);

        mSize++;
    }

    // x, y에 대한 노드를 얻는다
    Node* GetNodeOrNull(int x, int y, int& outIndex)
    {
        outIndex = -1;

        for (int i = 0; i < mSize; ++i)
        {
            if (mDatas[i]->X == x && mDatas[i]->Y == y)
            {
                outIndex = i;
                return mDatas[i];
            }
        }

        return nullptr;
    }

    // 힙을 보수한다
    // Push() 내부에서도 호출되고, 외부에서 값을 변경하고 이 함수를 호출해도 됨
    // 단, 우선순위를 낮추는 것에 대해서는 정상적인 동작을 보장하지 않음 (힙의 위로만 올라감)
    void RepairHeap(int index)
    {
        int currentIndex = index;
        int parentIndex = (currentIndex - 1) / 2;

        while (currentIndex != parentIndex)
        {
            if (*(mDatas[parentIndex]) < *(mDatas[currentIndex]))
            {
                Node* temp = mDatas[parentIndex];
                mDatas[parentIndex] = mDatas[currentIndex];
                mDatas[currentIndex] = temp;

                currentIndex = parentIndex;
                parentIndex = (parentIndex - 1) / 2;
            }
            else
            {
                break;
            }
        }
    }

    // 우선순위가 가장 높은 노드를 삭제한다
    void Pop()
    {
        assert(mSize > 0);
        mDatas[0] = mDatas[mSize - 1];
        mSize--;

        if (mSize <= 1)
        {
            return;
        }

        int currentIndex = 0;
        int maxIndex = (int)mSize - 1;
        while (true)
        {
            int leftChildIndex = currentIndex * 2 + 1;
            int rightChildIndex = currentIndex * 2 + 2;

            if (rightChildIndex <= maxIndex)
            {
                if (*(mDatas[currentIndex]) > *(mDatas[leftChildIndex]) && *(mDatas[currentIndex]) > *(mDatas[rightChildIndex]))
                {
                    break;
                }
                else if (*(mDatas[leftChildIndex]) >= *(mDatas[rightChildIndex]))
                {
                    Node* temp = mDatas[leftChildIndex];
                    mDatas[leftChildIndex] = mDatas[currentIndex];
                    mDatas[currentIndex] = temp;

                    currentIndex = leftChildIndex;
                }
                else
                {
                    Node* temp = mDatas[rightChildIndex];
                    mDatas[rightChildIndex] = mDatas[currentIndex];
                    mDatas[currentIndex] = temp;

                    currentIndex = rightChildIndex;
                }
            }
            else if (leftChildIndex <= maxIndex)
            {
                if (*(mDatas[leftChildIndex]) > *(mDatas[currentIndex]))
                {
                    Node* temp = mDatas[leftChildIndex];
                    mDatas[leftChildIndex] = mDatas[currentIndex];
                    mDatas[currentIndex] = temp;
                }

                break;
            }
            else
            {
                break;
            }
        }
    }

    inline Node* Top() const
    {
        assert(mSize > 0);
        return mDatas[0];
    }

    inline int Size() const
    {
        return mSize;
    }

    inline bool Empty() const
    {
        return mSize == 0;
    }

    inline void Clear()
    {
        for (int i = 0; i < mSize; ++i)
        {
            delete mDatas[i];
        }

        mSize = 0;
    }
private:
    Node** mDatas;
    int mSize;
    int mCapacity;
};