#pragma once

#include <memory>

namespace Traits
{
	class IBatchable
	{
	public:
		IBatchable()
			: m_dirty(false)
		{

		}

		virtual ~IBatchable() = default;

		virtual void Commit() = 0;
		virtual void Rollback() noexcept = 0;

		// This can be overridden
		virtual void OnInitWrite(const bool /*batch*/) { }

		// This can be overridden
		virtual void OnEndWrite() { }

		bool IsDirty() const noexcept { return m_dirty; }
		void SetDirty(const bool dirty) noexcept { m_dirty = dirty; }

	private:
		bool m_dirty;
	};
}