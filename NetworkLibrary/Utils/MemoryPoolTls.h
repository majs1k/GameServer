#pragma once
#include <iostream>
#include <vector>
#include <mutex>

#define CHUNK_SIZE			800


// MemoryPoolTls가 템플릿 클래스임을 미리 선언
template <class T>
class MemoryPoolTls;


template <class T>
struct MemoryPoolNode
{
	T data_;

	MemoryPoolNode* next_;
};


template <class T>
class MemoryPool
{
	friend class MemoryPoolTls<T>;

	//typedef MemoryPoolNode<T> Node;
	using Node = MemoryPoolNode<T>;

private:

	Node* top_ = nullptr;

	int size_ = 0;


	Node* freeHead_ = nullptr;
	Node* freeTail_ = nullptr;

	int freeSize_ = 0;

	bool isPlacementNew_ = false;


public:
	MemoryPool(bool isPlacementNew = false)
	{
		isPlacementNew_ = isPlacementNew;

		MemoryPoolTls<T>::RegisterPool(this);
	}

	T* Alloc()
	{
		if (!top_)
		{
			if (!freeHead_)
			{
				MemoryPoolTls<T>::AcquireNodes(this);
			}
			else
			{
				freeTail_->next_ = top_;
				top_ = freeHead_;

				freeHead_ = nullptr;
				freeTail_ = nullptr;
			}
		}

		Node* node = top_;

		top_ = node->next_;
		node->next_ = nullptr;

		--size_;

		if (isPlacementNew_)
			new (&node->data_) T();

		return &node->data_;
	}

	bool Free(T* value)
	{
		Node* node = reinterpret_cast<Node*>(value);

		if (isPlacementNew_)
			value->~T();

		node->next_ = nullptr;

		// top_에 100개 미만의 블록이 있으면 우선 보관
		if (size_ < CHUNK_SIZE)
		{
			node->next_ = top_;
			top_ = node;
			++size_;

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

		++freeSize_;

		// 100개가 모이면 공용 저장소로 반환
		if (freeSize_ >= CHUNK_SIZE)
		{
			MemoryPoolTls<T>::ReturnNodes(freeHead_, freeTail_);

			freeHead_ = nullptr;
			freeTail_ = nullptr;
			freeSize_ = 0;
		}

		return true;
	}
};


template <class T>
class MemoryPoolTls
{
	typedef MemoryPoolNode<T> Node;


public:

	static std::vector<MemoryPool<T>*>& Pools()
	{
		static std::vector<MemoryPool<T>*> pools;

		return pools;
	}

	static void RegisterPool(MemoryPool<T>* pool)
	{
		std::lock_guard<std::mutex> guard(Lock());

		Pools().push_back(pool);
	}

	~MemoryPoolTls()
	{
		for (auto& i : Chunks())
		{
			Node* next = i.head;

			while (next != nullptr)
			{
				Node* del = next;
				next = next->next_;

				delete del;
			}
		}

		for (auto& i : Pools())
		{
			Node* next;

			next = i->top_;

			while (next != nullptr)
			{
				Node* del = next;
				next = next->next_;

				delete del;
			}

			next = i->freeHead_;

			while (next != nullptr)
			{
				Node* del = next;
				next = next->next_;

				delete del;
			}
		}
	}

private:

	struct Chunk
	{
		Node* head;
		Node* tail;
		int size;

		Chunk(Node* h, Node* t, int s)
			: head(h), tail(t), size(s)
		{
		}
	};

	static std::vector<Chunk>& Chunks()
	{
		static std::vector<Chunk> chunks;
		return chunks;
	}

	static std::mutex& Lock()
	{
		static std::mutex mutex;
		return mutex;
	}

	static MemoryPool<T>& LocalPool()
	{
		static thread_local MemoryPool<T> pool;
		return pool;
	}

	static Chunk CreateChunk()
	{
		Node* head = nullptr;
		Node* tail = nullptr;

		for (int count = 0; count < CHUNK_SIZE; ++count)
		{
			Node* node = new Node();

			if (!head)
				head = tail = node;
			else
			{
				tail->next_ = node;
				tail = node;
			}
		}

		return Chunk(head, tail, CHUNK_SIZE);
	}

public:

	static void AcquireNodes(MemoryPool<T>* pool)
	{
		Chunk chunk(nullptr, nullptr, 0);

		{
			std::lock_guard<std::mutex> guard(Lock());

			if (!Chunks().empty())
			{
				chunk = Chunks().back();
				Chunks().pop_back();
			}
		}

		// 공용 Chunk가 없으면 새로 생성
		if (!chunk.head)
			chunk = CreateChunk();

		chunk.tail->next_ = pool->top_;
		pool->top_ = chunk.head;

		pool->size_ += chunk.size;
	}

	static void ReturnNodes(Node* head, Node* tail)
	{
		std::lock_guard<std::mutex> guard(Lock());

		Chunks().push_back(Chunk(head, tail, CHUNK_SIZE));
	}

	static T* Alloc()
	{
		return LocalPool().Alloc();
	}

	static bool Free(T* value)
	{
		return LocalPool().Free(value);
	}
};
