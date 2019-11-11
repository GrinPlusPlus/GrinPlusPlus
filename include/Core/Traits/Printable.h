#pragma once

#include <string>

namespace Traits
{
	class IPrintable
	{
	public:
		virtual ~IPrintable() = default;

		virtual std::string Format() const = 0;
	};
}