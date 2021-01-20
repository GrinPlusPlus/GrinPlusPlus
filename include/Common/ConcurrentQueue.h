#pragma once

#include <deque>
#include <vector>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <condition_variable>

template <typename T>
class ConcurrentQueue
{
public:
	bool contains(const T& item) const
	{
		std::shared_lock<std::shared_mutex> readLock(m_mutex);

		for (auto iter = m_deque.cbegin(); iter != m_deque.cend(); iter++)
		{
			if (*iter == item)
			{
				return true;
			}
		}

		return false;
	}

	template <typename U>
	bool contains(const U& item, std::function<bool(const T&, const U&)>& comparator) const
	{
		std::shared_lock<std::shared_mutex> readLock(m_mutex);

		for (auto iter = m_deque.cbegin(); iter != m_deque.cend(); iter++)
		{
			if (comparator(*iter, item))
			{
				return true;
			}
		}

		return false;
	}

	std::vector<T> copy_front(const size_t numItems) const
	{
		std::shared_lock<std::shared_mutex> readLock(m_mutex);

		std::vector<T> items;
		items.reserve(numItems);

		for (auto iter = m_deque.cbegin(); iter != m_deque.cend(); iter++)
		{
			items.push_back(*iter);

			if (items.size() == numItems)
			{
				break;
			}
		}

		return items;
	}

	std::unique_ptr<T> copy_front()
	{
		std::shared_lock<std::shared_mutex> readLock(m_mutex);

		if (!m_deque.empty())
		{
			return std::make_unique<T>(m_deque.front());
		}

		return nullptr;
	}

	void pop_front(const size_t numItems)
	{
		std::unique_lock<std::shared_mutex> writeLock(m_mutex);

		size_t itemsPopped = 0;
		while (itemsPopped < numItems && !m_deque.empty())
		{
			m_deque.pop_front();
			++itemsPopped;
		}
	}

	//T pop()
	//{
	//	std::unique_lock<std::shared_mutex> writeLock(m_mutex);
	//	while (m_deque.empty())
	//	{
	//		m_conditional.wait(writeLock);
	//	}
	//	auto item = m_deque.front();
	//	m_deque.pop_front();
	//	return item;
	//}

	//void pop(T& item)
	//{
	//	std::unique_lock<std::shared_mutex> writeLock(m_mutex);
	//	while (m_deque.empty())
	//	{
	//		m_conditional.wait(writeLock);
	//	}
	//	item = m_deque.front();
	//	m_deque.pop_front();
	//}

	void push_back(const T& item)
	{
		std::unique_lock<std::shared_mutex> writeLock(m_mutex);
		m_deque.push_back(item);
		writeLock.unlock();
		m_conditional.notify_one();
	}

	void push_back(T&& item)
	{
		std::unique_lock<std::shared_mutex> writeLock(m_mutex);
		m_deque.push_back(std::move(item));
		writeLock.unlock();
		m_conditional.notify_one();
	}

	bool push_back_unique(T&& item, std::function<bool(const T&, const T&)>& comparator)
	{
		std::unique_lock<std::shared_mutex> writeLock(m_mutex);

		for (auto iter = m_deque.cbegin(); iter != m_deque.cend(); iter++)
		{
			if (comparator(*iter, item))
			{
				return false;
			}
		}

		m_deque.push_back(std::move(item));
		return true;
	}

	size_t size() const noexcept
	{
		std::shared_lock<std::shared_mutex> read_lock(m_mutex);
		return m_deque.size();
	}

private:
	std::deque<T> m_deque;
	mutable std::shared_mutex m_mutex;
	std::condition_variable m_conditional;
};