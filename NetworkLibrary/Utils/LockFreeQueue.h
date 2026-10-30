#pragma once
#include <iostream>
#include <Windows.h>

#include "MemoryPoolTls.h"

#include "LockFreeNode.h"

//struct alignas(16) Log
//{
//	unsigned short front_ = 0;
//
//	unsigned short rear_ = 0;
//
//	unsigned short next_ = 0;
//
//	unsigned short node_ = 0;
//
//	unsigned short ret_ = 0;
//
//	unsigned short type_ = 0;
//
//	unsigned short threadId_ = 0;
//};
//
//#define MAX_INDEX		1000000
//Log logger[MAX_INDEX];
//alignas(64) uint64_t idx;



template <typename T>
class LockFreeQueue
{
private:

	using Node = LockFreeNode<T>;

	static constexpr uint64_t PTR_MASK = 0x0000FFFFFFFFFFFF;
	static constexpr uint64_t TAG_MASK = 0xFFFF000000000000;

	static uint64_t MakeAddress(Node* node, uint64_t tag)
	{
		return (reinterpret_cast<uint64_t>(node) & PTR_MASK) | ((tag & 0xFFFF) << 48);
	}

	static Node* GetPtr(uint64_t value)
	{
		return reinterpret_cast<Node*>(value & PTR_MASK);
	}

	static uint64_t GetTag(uint64_t value)
	{
		return (value & TAG_MASK) >> 48;
	}

private:

	uint64_t front_ = 0;
	uint64_t rear_ = 0;

	unsigned int size_ = 0;



public:

	LockFreeQueue()
	{
		//Node* node = new Node();
		Node* node = pool_<T>.Alloc();

		uint64_t dummy = MakeAddress(node, 1);

		front_ = dummy;
		rear_ = dummy;

		node->next_ = 0;
	}

	void Enqueue(T value)
	{
		//Node* node = new Node();
		Node* node = pool_<T>.Alloc();

		node->Initialize(value);

		uint64_t nodeAddr;

		//Log log;
		//DWORD tid = GetCurrentThreadId();
		//int i;

		while (1)
		{
			uint64_t front = front_;

			uint64_t rear = rear_;

			uint64_t next = GetPtr(rear)->next_;

			if (next == 0)
			{
				uint64_t newTag = GetTag(rear) + 1;

				nodeAddr = MakeAddress(node, newTag);

				//i = InterlockedIncrement(&idx) % MAX_INDEX;
				//log.front_ = (unsigned short)front;
				//log.rear_ = (unsigned short)rear;
				//log.next_ = (unsigned short)next;
				//log.node_ = (unsigned short)node;
				//log.threadId_ = (unsigned short)tid;
				//log.type_ = (unsigned short)0x11;
				//logger[i] = log;

				if (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&(GetPtr(rear)->next_)), nodeAddr, 0) == 0)
				{
					int64_t rr = InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&rear_), nodeAddr, rear);

					break;
				}
			}
			else
			{
				InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&rear_), next, rear);
			}
		}

		InterlockedIncrement(&size_);
	}

	bool Dequeue(T& value)
	{
		if (size_ == 0)
		{
			//__debugbreak();

			return false;
		}

		uint64_t front;
		uint64_t rear;
		uint64_t next;
		uint64_t newTag;

		Node* frontPtr;

		//Log log;
		//DWORD tid = GetCurrentThreadId();
		//int i;

		while (1)
		{
			front = front_;
			rear = rear_;

			frontPtr = GetPtr(front);

			newTag = GetTag(front) + 1;
			next = MakeAddress(GetPtr(frontPtr->next_), newTag);

			if (GetPtr(rear)->next_ != 0)
			{
				InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&rear_), GetPtr(rear)->next_, rear);
			}

			if (GetPtr(frontPtr->next_) == 0)
			{
				return false;
			}
			else
			{
				if (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&front_), next, front) == front)
				{
					Node* n = GetPtr(next);
					value = n->data_;

					//delete frontPtr;
					pool_<T>.Free(frontPtr);

					break;
				}
			}
		}

		InterlockedDecrement(&size_);

		return true;
	}

	int Size() const
	{
		return size_;
	}

	void Clear()
	{
		uint64_t current = front_;
		Node* node = GetPtr(current);

		// 현재 더미 노드부터 연결된 노드를 순회하며 해제
		while (1)
		{
			uint64_t next = node->next_;

			if (next == 0)
			{
				front_ = rear_ = current;
				break;
			}

			//delete node;
			pool_<T>.Free(node);

			current = next;
			node = GetPtr(next);
		}

		size_ = 0;
	}
};
