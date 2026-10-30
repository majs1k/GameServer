#pragma once
#include <queue>
#include <mutex>
#include "Packet.h"

class MessageQueue
{
private:

	std::queue<Message> queue_;
	std::mutex lock_;

public:

	void Push(Message message)
	{
		lock_.lock();

		queue_.push(message);

		lock_.unlock();
	}

	bool Pop(Message& message)
	{
		lock_.lock();

		if (queue_.empty())
		{
			lock_.unlock();

			return false;
		}

		message = queue_.front();

		queue_.pop();

		lock_.unlock();

		return true;
	}
};
