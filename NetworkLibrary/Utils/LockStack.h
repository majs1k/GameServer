#pragma once
#include <stack>
#include <mutex>


template<typename T>
class LockStack
{
public:

	void Push(T value)
	{
		std::lock_guard<std::mutex> lock(lock_);

		stack_.push(value);
	}

	bool Pop(T& value)
	{
		std::lock_guard<std::mutex> lock(lock_);

		if (stack_.empty())
			return false;
		
		value = stack_.top();
		stack_.pop();
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
