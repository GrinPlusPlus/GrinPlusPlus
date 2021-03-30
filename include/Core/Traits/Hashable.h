#pragma once

#include <Crypto/Models/Hash.h>

namespace Traits
{
	class IHashable
	{
	public:
		virtual ~IHashable() = default;

		virtual const Hash& GetHash() const = 0;
	};
}