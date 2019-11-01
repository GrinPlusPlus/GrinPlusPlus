#pragma once

#include <memory>

namespace Traits
{
	class Batchable
	{
	public:
		Batchable()
			: m_dirty(false)
		{

		}

		virtual ~Batchable() = default;

		virtual void Commit() = 0;
		virtual void Rollback() = 0;

		// This can be overridden
		virtual void OnInitWrite() { }

		// This can be overridden
		virtual void OnEndWrite() { }

		bool IsDirty() const { return m_dirty; }
		void SetDirty(const bool dirty) { m_dirty = dirty; }

	private:
		bool m_dirty;
	};

	//class IBatch
	//{
	//public:
	//	IBatch() : m_dirty(false) { }

	//	virtual ~IBatch()
	//	{
	//		if (m_dirty)
	//		{
	//			Rollback();
	//		}
	//	}

	//	virtual void Commit() = 0;
	//	virtual void Rollback() = 0;

	//	bool IsDirty() const { return m_dirty; }
	//	void SetDirty(const bool dirty) { m_dirty = dirty; }

	//private:
	//	bool m_dirty;
	//};

	//template<typename T, typename = std::enable_if_t<std::is_base_of<Traits::IBatch, T>::value>>
	//class IBatchable
	//{
	//public:
	//	virtual ~IBatchable() = default;

	//	virtual const std::shared_ptr<T> Read() const = 0;
	//	virtual std::shared_ptr<T> Write() = 0;
	//};
}