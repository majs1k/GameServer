#pragma once
#include <iostream>
#include <Windows.h>

#include "MemoryPoolTls.h"

#include "LockFreeNode.h"


//#define MAX_INDEX	1000000

//struct alignas(16) Log
//{
//	unsigned short t_;
//
//	unsigned short tnext_;
//
//	unsigned short type_;
//
//	unsigned short threadId_;
//};
//
//Log logger[MAX_INDEX];
//alignas(64) uint64_t idx;


template <typename T>
class LockFreeStack
{
private:

	using Node = LockFreeNode<T>;

	static constexpr uint64_t PTR_MASK = 0x0000FFFFFFFFFFFF;
	static constexpr uint64_t TAG_MASK = 0xFFFF000000000000;

	static uint64_t MakeAddress(Node* node, uint64_t tag)
	{
		return (reinterpret_cast<uint64_t>(node) & PTR_MASK) | ((tag & 0x1FFFFULL) << 48);
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

	uint64_t top_ = 0;

	LONG size_ = 0;


public:

	void Push(T value)
	{
		//Node* node = new Node();
		Node* node = pool_<T>.Alloc();
		node->Initialize(value);

		uint64_t top;
		uint64_t nodeAddr;

		//Log log;
		//DWORD tid = GetCurrentThreadId();
		//int i;

		do
		{
			top = top_;

			node->next_ = top;

			uint64_t newTag = GetTag(top) + 1;

			nodeAddr = MakeAddress(node, newTag);

			//i = InterlockedIncrement(&idx) % MAX_INDEX;
			//log.t_ = (unsigned short)top;
			//log.top_ = (unsigned short)top_;
			//log.threadId_ = (unsigned short)tid;
			//log.type_ = (unsigned short)0x11;
			//logger[i] = log;

		} while (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&top_), nodeAddr, top) != top);

		InterlockedIncrement(&size_);
	}

	bool Pop(T& value)
	{
		if (size_ == 0)
			return false;

		uint64_t top;
		uint64_t next;
		Node* topPtr;

		do
		{
			top = top_;

			if (GetPtr(top) == nullptr)
				return false;

			topPtr = GetPtr(top);

			uint64_t newTag = GetTag(top) + 1;

			next = MakeAddress(GetPtr(topPtr->next_), newTag);

		} while (InterlockedCompareExchange64(reinterpret_cast<LONG64*>(&top_), next, top) != top);

		value = topPtr->data_;

		//delete topPtr;
		pool_<T>.Free(topPtr);

		InterlockedDecrement(&size_);

		return true;
	}

	int Size() const
	{
		return size_;
	}

	void Clear()
	{
		uint64_t current = top_;
		Node* node = GetPtr(current);

		while (node != nullptr)
		{
			uint64_t next = node->next_;

			//delete node;
			pool_<T>.Free(node);

			node = GetPtr(next);
		}

		top_ = 0;
		size_ = 0;
	}
};
