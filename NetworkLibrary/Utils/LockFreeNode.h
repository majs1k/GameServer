#pragma once
#include "MemoryPoolTls.h"


template <typename T>
struct LockFreeNode
{
	T data_;
	uint64_t next_;

	LockFreeNode() = default;

	void Initialize(T value)
	{
		data_ = value;
		next_ = 0;
	}
};

template <typename T>
inline MemoryPoolTls<LockFreeNode<T>> pool_;
