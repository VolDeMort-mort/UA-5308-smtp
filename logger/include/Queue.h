#pragma once

#include <optional>

template <typename T, size_t capacity>
class Queue
{
public:
	Queue() : m_head(0), m_tail(0), m_head_reserve(0)
	{
		static_assert(capacity >= 2, "Queue capacity must be at least 2");
		static_assert((capacity & (capacity - 1)) == 0, "Queue capacity must be power of 2");
	}

	bool Push(T& log)
	{
		unsigned reserveHead = m_head_reserve.load(std::memory_order_relaxed);
		while (true)
		{
			unsigned currentTail = m_tail.load(std::memory_order_acquire);

			if (reserveHead - currentTail >= capacity)
			{
				std::this_thread::yield();
				reserveHead = m_head_reserve.load(std::memory_order_relaxed);
				continue;
			}

			// incrementing m_head_reserve & add log to buffer
			if (m_head_reserve.compare_exchange_weak(reserveHead, reserveHead + 1, std::memory_order_relaxed))
			{
				m_ringBuffer[reserveHead & s_maxIndexMask] = std::move(log); // if reserveHead > capacity-1, buffer index = 0

				unsigned expected = reserveHead;

				// incrementing m_head, allowing the reader to safety taking correct log from queue
				while (!m_head.compare_exchange_weak(expected, reserveHead + 1, std::memory_order_release, std::memory_order_relaxed))
				{
					expected = reserveHead;
					std::this_thread::yield();
				}

				return true;
			}
		}
	}
	std::optional<T> Pop()
	{
		unsigned currentTail = m_tail.load(std::memory_order_relaxed);
		unsigned currentHead = m_head.load(std::memory_order_relaxed);

		if (currentTail == currentHead)
			return std::nullopt;

		T result = std::move(m_ringBuffer[currentTail & s_maxIndexMask]);

		m_tail.store(currentTail + 1, std::memory_order_release);

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
	alignas(64) std::atomic<unsigned> m_head_reserve;

	T m_ringBuffer[capacity];
	static constexpr unsigned s_maxIndexMask = capacity - 1;
};