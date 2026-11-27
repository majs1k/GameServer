#pragma once

#ifdef _MSC_VER
#include <winsock2.h>
#endif

#include <hiredis.h>
#include <string>

class Redis
{
public:

	Redis() = default;

	~Redis()
	{
		Disconnect();
	}

	Redis(const Redis&) = delete;
	Redis& operator=(const Redis&) = delete;

	void Connect(std::string host, int port)
	{
		Disconnect();

		context_ = redisConnect(host.c_str(), port);

		if (context_ == nullptr || context_->err)
		{
			Disconnect();
			__debugbreak();
		}
	}

	void Disconnect()
	{
		if (context_ != nullptr)
		{
			redisFree(context_);
			context_ = nullptr;
		}
	}

	bool IsConnected() const
	{
		return context_ != nullptr && context_->err == 0;
	}

	/// TODO: 수정
	bool Set(const std::string& key, const std::string& value, bool onlyNotExist = false, int expireSeconds = 0)
	{
		if (!IsConnected())
			return false;

		redisReply* reply = nullptr;

		if (onlyNotExist && expireSeconds > 0)
		{
			// Key가 없을 때만 SET + 만료 시간
			reply = static_cast<redisReply*>(
				redisCommand(context_, "SET %b %b NX EX %d",
					key.data(), key.size(),
					value.data(), value.size(),
					expireSeconds)
				);
		}
		else if (onlyNotExist)
		{
			// Key가 없을 때만 SET
			reply = static_cast<redisReply*>(
				redisCommand(context_, "SET %b %b NX",
					key.data(), key.size(),
					value.data(), value.size())
				);
		}
		else if (expireSeconds > 0)
		{
			// SET + 만료 시간
			reply = static_cast<redisReply*>(
				redisCommand(context_, "SET %b %b EX %d",
					key.data(), key.size(),
					value.data(), value.size(),
					expireSeconds)
				);
		}
		else
		{
			// 일반 SET
			reply = static_cast<redisReply*>(
				redisCommand(context_, "SET %b %b",
					key.data(), key.size(),
					value.data(), value.size())
				);
		}

		if (reply == nullptr)
		{
			__debugbreak();
			return false;
		}

		bool result = (reply->type == REDIS_REPLY_STATUS &&
			std::string(reply->str) == "OK");

		freeReplyObject(reply);

		return result;
	}

	bool Get(const std::string& key, std::string& value)
	{
		if (!IsConnected())
			return false;

		redisReply* reply = static_cast<redisReply*>(
			redisCommand(context_, "GET %b",
				key.data(), key.size())
			);

		if (reply == nullptr)
			return false;

		bool result = false;

		if (reply->type == REDIS_REPLY_STRING)
		{
			value.assign(reply->str, reply->len);
			result = true;
		}

		freeReplyObject(reply);

		return result;
	}

	bool Del(const std::string& key)
	{
		if (!IsConnected())
			return false;

		redisReply* reply = static_cast<redisReply*>(
			redisCommand(context_, "DEL %b",
				key.data(), key.size())
			);

		if (reply == nullptr)
			return false;

		bool result = (reply->type == REDIS_REPLY_INTEGER &&
			reply->integer == 1);

		freeReplyObject(reply);

		return result;
	}

private:

	redisContext* context_ = nullptr;
};
