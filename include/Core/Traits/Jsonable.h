#pragma once

#include <json/json.h>

namespace Traits
{
	class IJsonable
	{
	public:
		virtual ~IJsonable() = default;

		virtual Json::Value ToJSON() const = 0;
	};
}