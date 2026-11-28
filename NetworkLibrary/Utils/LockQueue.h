#pragma once
#include <queue>
#include <mutex>


template<typename T>
class LockQueue
{
public:

	void Enqueue(T value)
	{
		std::lock_guard<std::mutex> lock(lock_);

		queue_.push(value);
	}

	bool Dequeue(T& value)
	{
		std::lock_guard<std::mutex> lock(lock_);

		if (queue_.empty())
			return false;

		value = queue_.front();
		queue_.pop();
		return true;
	}

	int Size() const
	{
		return queue_.size();
	}

private:

	std::queue<T> queue_;
	std::mutex lock_;
};
