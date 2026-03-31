#pragma once

#include <optional>

template <typename T, size_t capacity>
class Queue
{
public:
	static_assert(capacity >= 2, "Queue capacity must be at least 2");
	Queue() : m_head(0), m_tail(0) {}

	bool Push(T& log)
	{
		while (IsFull())
			std::this_thread::yield();

		unsigned currentHead = m_head.load(std::memory_order_acquire);
		unsigned nextHead = (currentHead + 1) % capacity;

		m_ringBuffer[currentHead] = std::move(log);

		m_head.store(nextHead, std::memory_order_release);
		return true;
	}
	std::optional<T> Pop()
	{
		unsigned currentTail = m_tail.load(std::memory_order_acquire);
		unsigned currentHead = m_head.load(std::memory_order_acquire);

		if (currentTail == currentHead)
			return std::nullopt;

		T result = std::move(m_ringBuffer[currentTail]);

		unsigned nextTail = (currentTail + 1) % capacity;
		m_tail.store(nextTail, std::memory_order_release);

		return result;
	}
	bool IsEmpty()
	{
		return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
	}
	bool IsFull()
	{
		return ((m_head.load(std::memory_order_acquire) + 1) % capacity) == m_tail.load(std::memory_order_acquire);
	}
private:
	alignas(64) std::atomic<unsigned> m_head; // writingIndex
	alignas(64) std::atomic<unsigned> m_tail; // readingIndex

	T m_ringBuffer[capacity];
};