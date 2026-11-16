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

	int Size() const
	{
		return queue_.size();
	}
};

#include <stack>

template<typename T>
class LockStack
{
private:

	std::stack<T> stack_;
	std::mutex lock_;

public:

	void Push(T value)
	{
		lock_.lock();

		stack_.push(value);

		lock_.unlock();
	}

	bool Pop(T& value)
	{
		lock_.lock();

		if (stack_.empty())
		{
			lock_.unlock();

			return false;
		}

		value = stack_.top();

		stack_.pop();

		lock_.unlock();

		return true;
	}

	int Size() const
	{
		return stack_.size();
	}
};
