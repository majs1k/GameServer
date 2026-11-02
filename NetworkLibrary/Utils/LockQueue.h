#pragma once
#include <queue>
#include <mutex>
#include "Packet.h"

template<typename T>
class LockQueue
{
private:

	std::queue<T> queue_;
	std::mutex lock_;

public:

	void Enqueue(T value)
	{
		lock_.lock();

		queue_.push(value);

		lock_.unlock();
	}

	bool Dequeue(T& value)
	{
		lock_.lock();

		if (queue_.empty())
		{
			lock_.unlock();

			return false;
		}

		value = queue_.front();

		queue_.pop();

		lock_.unlock();

		return true;
	}
};
