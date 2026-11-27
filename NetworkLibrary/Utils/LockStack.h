#pragma once
#include <stack>
#include <mutex>


template<typename T>
class LockStack
{
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

private:

	std::stack<T> stack_;
	std::mutex lock_;
};
