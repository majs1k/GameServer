#pragma once

#include <iostream>
#include <vector>
#include <mutex>

#define CHUNK_SIZE				400


template <class T>
class MemoryPoolTls
{
private:

	struct Node
	{
		T data_;
		Node* next_ = nullptr;
	};

	struct Chunk
	{
		Node* head_ = nullptr;
		Node* tail_ = nullptr;

		Chunk() = default;

		Chunk(Node* h, Node* t)
			: head_(h), tail_(t)
		{
		}
	};

	struct MemoryPool
	{
		Node* top_ = nullptr;
		int size_ = 0;
		Node* freeHead_ = nullptr;
		Node* freeTail_ = nullptr;
		int freeSize_ = 0;
		MemoryPoolTls<T>* poolTls_ = nullptr;

	public:

		MemoryPool(MemoryPoolTls<T>* poolTls)
		{
			poolTls_ = poolTls;

			poolTls_->lock_.lock();

			poolTls_->pools_.push_back(this);

			poolTls_->lock_.unlock();
		}

		~MemoryPool()
		{

		}

		T* Alloc()
		{
			if (!top_)
			{
				if (!freeHead_)
				{
					Chunk newChunk;
					poolTls_->AllocChunk(newChunk);

					newChunk.tail_->next_ = top_;
					top_ = newChunk.head_;

					size_ += CHUNK_SIZE;
				}
				else
				{
					freeTail_->next_ = top_;
					top_ = freeHead_;
					size_ += freeSize_;

					freeHead_ = nullptr;
					freeTail_ = nullptr;
					freeSize_ = 0;
				}
			}

			Node* node = top_;

			top_ = node->next_;
			node->next_ = nullptr;

			size_--;

			return &node->data_;
		}

		bool Free(T* value)
		{
			if (value == nullptr)
				return false;

			Node* node = reinterpret_cast<Node*>(value);

			node->next_ = nullptr;

			// top_에 CHUNK_SIZE 미만의 블록이 있으면 우선 보관
			if (size_ < CHUNK_SIZE)
			{
				node->next_ = top_;
				top_ = node;
				size_++;

				return true;
			}

			// top_이 가득 찼으면 freeList에 보관
			if (!freeHead_)
				freeHead_ = freeTail_ = node;
			else
			{
				freeTail_->next_ = node;
				freeTail_ = node;
			}

			freeSize_++;

			// 일정량이 모이면 공용 저장소로 반환
			if (freeSize_ >= CHUNK_SIZE)
			{
				Chunk chunk{ freeHead_, freeTail_ };
				poolTls_->ReturnChunk(chunk);

				freeHead_ = nullptr;
				freeTail_ = nullptr;
				freeSize_ = 0;
			}

			return true;
		}
	};

private:

	std::vector<MemoryPool*> pools_{};
	std::vector<Chunk> chunks_{};
	std::mutex lock_{};
	int capacity_ = 0;

public:

	~MemoryPoolTls()
	{
		// 공용 Chunk 해제
		for (auto& chunk : chunks_)
		{
			Node* cur = chunk.head_;

			while (cur != nullptr)
			{
				Node* delNode = cur;
				cur = cur->next_;

				delete delNode;
			}
		}

		// 등록된 Pool의 노드 해제
		for (auto& pool : pools_)
		{
			Node* cur = pool->top_;

			while (cur != nullptr)
			{
				Node* delNode = cur;
				cur = cur->next_;

				delete delNode;
			}

			cur = pool->freeHead_;

			while (cur != nullptr)
			{
				Node* delNode = cur;
				cur = cur->next_;

				delete delNode;
			}
		}
	}

	T* Alloc()
	{
		return LocalPool().Alloc();
	}

	bool Free(T* value)
	{
		return LocalPool().Free(value);
	}

	int Capacity()
	{
		return capacity_;
	}

private:

	MemoryPool& LocalPool()
	{
		// MemoryPoolTls 인스턴스마다 pool이 별개로 존재함
		// 락프리 자료구조를 위해 LockFreeNode 추가 필요
		static thread_local MemoryPool pool{ this };
		return pool;
	}

	Chunk CreateChunk()
	{
		Node* head = nullptr;
		Node* tail = nullptr;

		for (int count = 0; count < CHUNK_SIZE; count++)
		{
			Node* node = new Node();

			if (!head)
			{
				head = tail = node;
			}
			else
			{
				tail->next_ = node;
				tail = node;
			}
		}

		capacity_ += CHUNK_SIZE;

		return Chunk{ head, tail };
	}

	void AllocChunk(Chunk& chunk)
	{
		{
			std::lock_guard<std::mutex> lock(lock_);

			// chunk가 있으면 획득
			if (!chunks_.empty())
			{
				chunk = chunks_.back();
				chunks_.pop_back();
			}
		}

		// chunk가 없으면 새로 생성
		if (!chunk.head_)
		{
			chunk = CreateChunk();
		}
	}

	void ReturnChunk(Chunk& chunk)
	{
		std::lock_guard<std::mutex> lock(lock_);

		chunks_.push_back(chunk);
	}
};
