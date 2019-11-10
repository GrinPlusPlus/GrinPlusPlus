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
}